#include "SpiAnalyzer.h"
#include "SpiAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

SpiAnalyzer::SpiAnalyzer()
   : Analyzer2(), mSettings(new SpiAnalyzerSettings()), mSimulationInitialized(false),
     mClock(NULL), mMosi(NULL), mMiso(NULL), mEnable(NULL)
{
   SetAnalyzerSettings(mSettings.get());
   UseFrameV2();
}

SpiAnalyzer::~SpiAnalyzer() { KillThread(); }

void SpiAnalyzer::SetupResults()
{
   mResults.reset(new SpiAnalyzerResults(this, mSettings.get()));
   SetAnalyzerResults(mResults.get());
   if (mSettings->mMosiChannel != UNDEFINED_CHANNEL)
      mResults->AddChannelBubblesWillAppearOn(mSettings->mMosiChannel);
   if (mSettings->mMisoChannel != UNDEFINED_CHANNEL)
      mResults->AddChannelBubblesWillAppearOn(mSettings->mMisoChannel);
   mResults->AddChannelBubblesWillAppearOn(mSettings->mMuxEnableChannel);
}

void SpiAnalyzer::Setup()
{
   mClock = GetAnalyzerChannelData(mSettings->mClockChannel);
   mEnable = GetAnalyzerChannelData(mSettings->mMuxEnableChannel);
   mMosi = mSettings->mMosiChannel == UNDEFINED_CHANNEL
              ? NULL : GetAnalyzerChannelData(mSettings->mMosiChannel);
   mMiso = mSettings->mMisoChannel == UNDEFINED_CHANNEL
              ? NULL : GetAnalyzerChannelData(mSettings->mMisoChannel);
   for (U32 i = 0; i < 4; ++i)
      mAddress[i] = GetAnalyzerChannelData(mSettings->mMuxAddressChannels[i]);
}

U32 SpiAnalyzer::ReadMuxAddress(U64 sample)
{
   U32 address = 0;
   for (U32 i = 0; i < 4; ++i) {
      mAddress[i]->AdvanceToAbsPosition(sample);
      if (mAddress[i]->GetBitState() == BIT_HIGH) address |= (1U << i);
   }
   return address;
}

bool SpiAnalyzer::EdgeBefore(U64 sample, AnalyzerChannelData *channel)
{
   return channel->DoMoreTransitionsExistInCurrentData() &&
          channel->GetSampleOfNextEdge() <= sample;
}

bool SpiAnalyzer::AddressChangedBefore(U64 sample)
{
   for (U32 i = 0; i < 4; ++i)
      if (EdgeBefore(sample, mAddress[i])) return true;
   return false;
}

void SpiAnalyzer::EmitError(U64 start, U64 end, U32 cs)
{
   if (end < start) end = start;
   Frame frame = {};
   frame.mStartingSampleInclusive = start;
   frame.mEndingSampleInclusive = end;
   frame.mFlags = SPI_ERROR_FLAG | DISPLAY_AS_ERROR_FLAG;
   frame.mData1 = cs;
   mResults->AddFrame(frame);
   FrameV2 event;
   U8 csByte = U8(cs);
   event.AddByteArray("cs", &csByte, 1);
   mResults->AddFrameV2(event, "error", start, end + 1);
   mResults->AddMarker(start, AnalyzerResults::ErrorSquare, mSettings->mClockChannel);
   mResults->CommitResults();
}

bool SpiAnalyzer::NextEnableStart(U64 *sample)
{
   // Start only on an inactive-to-active transition, never on an arbitrary clock edge.
   for (;;) {
      CheckIfThreadShouldExit();
      if (!mEnable->DoMoreTransitionsExistInCurrentData()) return false;
      mEnable->AdvanceToNextEdge();
      if (mEnable->GetBitState() == BIT_LOW) {
         *sample = mEnable->GetSampleNumber();
         return true;
      }
   }
}

void SpiAnalyzer::DecodeTransaction(U64 start, U32 cs)
{
   const SpiClientSettings &cfg = mSettings->mClients[cs];
   const U32 bits = cfg.bitsPerTransfer;
   if (bits < 1 || bits > 64) return;
   const U32 bytes = (bits + 7) / 8;
   mClock->AdvanceToAbsPosition(start);
   if (mClock->GetBitState() != cfg.clockIdle) {
      EmitError(start, start, cs);
      return;
   }
   FrameV2 begin;
   U8 csByte = U8(cs);
   begin.AddByteArray("cs", &csByte, 1);
   mResults->AddFrameV2(begin, "enable", start, start + 1);
   mResults->CommitPacketAndStartNewPacket();
   mResults->CommitResults();

   const AnalyzerResults::MarkerType marker =
      (cfg.clockIdle == BIT_LOW) ==
      (cfg.phase == AnalyzerEnums::LeadingEdge)
         ? AnalyzerResults::UpArrow : AnalyzerResults::DownArrow;

   for (;;) {
      CheckIfThreadShouldExit();
      if (!mEnable->DoMoreTransitionsExistInCurrentData()) return;
      U64 end = mEnable->GetSampleOfNextEdge();
      if (AddressChangedBefore(end)) {
         EmitError(start, end, cs);
         mEnable->AdvanceToAbsPosition(end);
         return;
      }
      U64 mosiWord = 0;
      U64 misoWord = 0;
      DataBuilder mosiBuilder;
      DataBuilder misoBuilder;
      mosiBuilder.Reset(&mosiWord, cfg.shiftOrder, bits);
      misoBuilder.Reset(&misoWord, cfg.shiftOrder, bits);
      U64 first = 0;
      U64 last = 0;
      bool complete = true;
      for (U32 bit = 0; bit < bits; ++bit) {
         CheckIfThreadShouldExit();
         // Two clock edges per bit. For CPHA=0 sample the first; for CPHA=1 the second.
         for (U32 half = 0; half < 2; ++half) {
            if (!mClock->DoMoreTransitionsExistInCurrentData()) return;
            U64 next = mClock->GetSampleOfNextEdge();
            if (next >= end || AddressChangedBefore(next)) {
               complete = false;
               break;
            }
            mClock->AdvanceToNextEdge();
            if (half == U32(cfg.phase == AnalyzerEnums::TrailingEdge)) {
               U64 sample = mClock->GetSampleNumber();
               if (bit == 0) first = sample;
               last = sample;
               if (mMosi != NULL) {
                  mMosi->AdvanceToAbsPosition(sample);
                  mosiBuilder.AddBit(mMosi->GetBitState());
               }
               if (mMiso != NULL) {
                  mMiso->AdvanceToAbsPosition(sample);
                  misoBuilder.AddBit(mMiso->GetBitState());
               }
               mResults->AddMarker(sample, marker, mSettings->mClockChannel);
            }
         }
         if (!complete) break;
      }
      if (!complete) {
         if (AddressChangedBefore(end)) EmitError(start, end, cs);
         mEnable->AdvanceToAbsPosition(end);
         break;
      }
      Frame frame = {};
      frame.mStartingSampleInclusive = first;
      frame.mEndingSampleInclusive = last;
      frame.mData1 = mosiWord;
      frame.mData2 = misoWord;
      frame.mFlags = (cs << 8);
      mResults->AddFrame(frame);
      U8 mosiBytes[8] = {};
      U8 misoBytes[8] = {};
      for (U32 i = 0; i < bytes; ++i) {
         const U32 shift = (bytes - i - 1) * 8;
         mosiBytes[i] = U8(mosiWord >> shift);
         misoBytes[i] = U8(misoWord >> shift);
      }
      FrameV2 result;
      result.AddByteArray("mosi", mosiBytes, bytes);
      result.AddByteArray("miso", misoBytes, bytes);
      result.AddByteArray("cs", &csByte, 1);
      mResults->AddFrameV2(result, "result", first, last + 1);
      mResults->CommitResults();
   }
   // One legacy frame spanning the entire active-low interval supplies the MUX_EN bubble.
   Frame selectFrame = {};
   selectFrame.mStartingSampleInclusive = start;
   selectFrame.mEndingSampleInclusive = mEnable->GetSampleNumber();
   selectFrame.mData1 = cs;
   selectFrame.mFlags = 0x10000;
   mResults->AddFrame(selectFrame);
   FrameV2 finish;
   finish.AddByteArray("cs", &csByte, 1);
   mResults->AddFrameV2(finish, "disable", mEnable->GetSampleNumber(),
                        mEnable->GetSampleNumber() + 1);
   mResults->CommitResults();
}

void SpiAnalyzer::WorkerThread()
{
   Setup();
   U64 start;
   while (NextEnableStart(&start)) {
      const U32 cs = ReadMuxAddress(start);
      DecodeTransaction(start, cs);
      ReportProgress(mEnable->GetSampleNumber());
   }
}

U32 SpiAnalyzer::GenerateSimulationData(U64 sample, U32 rate,
                                        SimulationChannelDescriptor **channels)
{
   if (!mSimulationInitialized) {
      mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
      mSimulationInitialized = true;
   }
   return mSimulationDataGenerator.GenerateSimulationData(sample, rate, channels);
}
U32 SpiAnalyzer::GetMinimumSampleRateHz() { return 10000; }
const char *SpiAnalyzer::GetAnalyzerName() const { return "SPI MUX (RB)"; }
bool SpiAnalyzer::NeedsRerun() { return false; }
const char *GetAnalyzerName() { return "SPI MUX (RB)"; }
Analyzer *CreateAnalyzer() { return new SpiAnalyzer(); }
void DestroyAnalyzer(Analyzer *analyzer) { delete analyzer; }
