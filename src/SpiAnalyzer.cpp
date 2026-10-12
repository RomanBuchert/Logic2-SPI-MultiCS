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

SpiAnalyzer::~SpiAnalyzer()
{
   KillThread();
}

void SpiAnalyzer::SetupResults()
{
   mResults.reset(new SpiAnalyzerResults(this, mSettings.get()));
   SetAnalyzerResults(mResults.get());

   if (mSettings->mMosiChannel != UNDEFINED_CHANNEL)
      mResults->AddChannelBubblesWillAppearOn(mSettings->mMosiChannel);
   if (mSettings->mMisoChannel != UNDEFINED_CHANNEL)
      mResults->AddChannelBubblesWillAppearOn(mSettings->mMisoChannel);
   if (mSettings->mMuxEnableChannel != UNDEFINED_CHANNEL)
      mResults->AddChannelBubblesWillAppearOn(mSettings->mMuxEnableChannel);
}

void SpiAnalyzer::SetupChannels()
{
   mClock = GetAnalyzerChannelData(mSettings->mClockChannel);
   mEnable = mSettings->mMuxEnableChannel == UNDEFINED_CHANNEL
                ? NULL : GetAnalyzerChannelData(mSettings->mMuxEnableChannel);
   mMosi = mSettings->mMosiChannel == UNDEFINED_CHANNEL
              ? NULL : GetAnalyzerChannelData(mSettings->mMosiChannel);
   mMiso = mSettings->mMisoChannel == UNDEFINED_CHANNEL
              ? NULL : GetAnalyzerChannelData(mSettings->mMisoChannel);

   for (U32 i = 0; i < 4; ++i)
      mAddress[i] = mSettings->mMuxAddressChannels[i] == UNDEFINED_CHANNEL
                       ? NULL : GetAnalyzerChannelData(mSettings->mMuxAddressChannels[i]);
}

U32 SpiAnalyzer::ReadAddress(U64 sample)
{
   U32 cs = 0;
   for (U32 i = 0; i < 4; ++i) {
      if (mAddress[i] == NULL)
         continue;
      mAddress[i]->AdvanceToAbsPosition(sample);
      if (mAddress[i]->GetBitState() == BIT_HIGH)
         cs |= 1U << i;
   }
   return cs;
}

void SpiAnalyzer::EmitWord(U32 cs, const DecodedWord &word)
{
   Frame legacy = {};
   legacy.mStartingSampleInclusive = word.start;
   legacy.mEndingSampleInclusive = word.end;
   legacy.mData1 = word.mosi;
   legacy.mData2 = word.miso;
   legacy.mFlags = U8(cs << 4);
   mResults->AddFrame(legacy);

   const U32 bits = mSettings->mClients[cs].bitsPerTransfer;
   const U32 byteCount = (bits + 7) / 8;
   U8 mosiBytes[8] = {};
   U8 misoBytes[8] = {};
   for (U32 i = 0; i < byteCount; ++i) {
      const U32 shift = (byteCount - 1 - i) * 8;
      mosiBytes[i] = U8(word.mosi >> shift);
      misoBytes[i] = U8(word.miso >> shift);
   }

   const U8 csByte = U8(cs);
   FrameV2 frame;
   frame.AddByteArray("mosi", mosiBytes, byteCount);
   frame.AddByteArray("miso", misoBytes, byteCount);
   frame.AddByteArray("cs", &csByte, 1);
   mResults->AddFrameV2(frame, "result", word.start, word.end + 1);
}

void SpiAnalyzer::DecodeSelection(U64 start, U64 end, U32 cs)
{
   const SpiClientSettings &cfg = mSettings->mClients[cs];
   const U32 wordBits = cfg.bitsPerTransfer;
   if (wordBits == 0 || wordBits > 64)
      return;

   const U8 csByte = U8(cs);
   FrameV2 enable;
   enable.AddByteArray("cs", &csByte, 1);
   mResults->AddFrameV2(enable, "enable", start, start + 1);

   // A clock transition is either leading or trailing relative to CPOL.
   // CPHA selects which transition samples data. Every sampled bit is appended
   // directly, so a word can end without waiting for another clock edge.
   mClock->AdvanceToAbsPosition(start);
   std::vector<DecodedWord> words;
   U32 bitCount = 0;
   U64 mosiWord = 0;
   U64 misoWord = 0;
   U64 firstSample = 0;
   U64 lastSample = 0;

   while (mClock->DoMoreTransitionsExistInCurrentData()) {
      CheckIfThreadShouldExit();
      const U64 nextClock = mClock->GetSampleOfNextEdge();
      if (nextClock >= end)
         break;

      mClock->AdvanceToNextEdge();
      const bool leading = (mClock->GetBitState() != cfg.clockIdle);
      const bool sampleEdge = cfg.phase == AnalyzerEnums::LeadingEdge ? leading : !leading;
      if (!sampleEdge)
         continue;

      const U64 sample = mClock->GetSampleNumber();
      mResults->AddMarker(sample, mClock->GetBitState() == BIT_HIGH
                                      ? AnalyzerResults::UpArrow
                                      : AnalyzerResults::DownArrow,
                          mSettings->mClockChannel);
      if (bitCount == 0)
         firstSample = sample;
      lastSample = sample;

      if (mMosi != NULL) {
         mMosi->AdvanceToAbsPosition(sample);
         const U64 bit = mMosi->GetBitState() == BIT_HIGH ? 1 : 0;
         if (cfg.shiftOrder == AnalyzerEnums::MsbFirst)
            mosiWord = (mosiWord << 1) | bit;
         else
            mosiWord |= bit << bitCount;
      }
      if (mMiso != NULL) {
         mMiso->AdvanceToAbsPosition(sample);
         const U64 bit = mMiso->GetBitState() == BIT_HIGH ? 1 : 0;
         if (cfg.shiftOrder == AnalyzerEnums::MsbFirst)
            misoWord = (misoWord << 1) | bit;
         else
            misoWord |= bit << bitCount;
      }

      ++bitCount;
      if (bitCount == wordBits) {
         DecodedWord word = {firstSample, lastSample, mosiWord, misoWord};
         words.push_back(word);
         bitCount = 0;
         mosiWord = 0;
         misoWord = 0;
      }
   }

   for (size_t i = 0; i < words.size(); ++i)
      EmitWord(cs, words[i]);

   FrameV2 disable;
   disable.AddByteArray("cs", &csByte, 1);
   mResults->AddFrameV2(disable, "disable", end, end + 1);
   mResults->CommitResults();
}

void SpiAnalyzer::WorkerThread()
{
   SetupChannels();

   if (mEnable == NULL) {
      // Without MUX_EN, decode continuous clock activity. Address bits are
      // sampled at the beginning of each word, allowing the CS to change.
      U32 bitCount = 0;
      U32 cs = 0;
      U64 mosiWord = 0;
      U64 misoWord = 0;
      U64 firstSample = 0;
      U64 lastSample = 0;
      while (mClock->DoMoreTransitionsExistInCurrentData()) {
         CheckIfThreadShouldExit();
         mClock->AdvanceToNextEdge();
         const U64 sample = mClock->GetSampleNumber();
         if (bitCount == 0)
            cs = ReadAddress(sample);
         const SpiClientSettings &cfg = mSettings->mClients[cs];
         const bool leading = mClock->GetBitState() != cfg.clockIdle;
         const bool sampleEdge = cfg.phase == AnalyzerEnums::LeadingEdge ? leading : !leading;
         if (!sampleEdge)
            continue;
         mResults->AddMarker(sample, mClock->GetBitState() == BIT_HIGH
                                         ? AnalyzerResults::UpArrow
                                         : AnalyzerResults::DownArrow,
                             mSettings->mClockChannel);
         if (bitCount == 0)
            firstSample = sample;
         lastSample = sample;
         if (mMosi != NULL) {
            mMosi->AdvanceToAbsPosition(sample);
            const U64 bit = mMosi->GetBitState() == BIT_HIGH ? 1 : 0;
            if (cfg.shiftOrder == AnalyzerEnums::MsbFirst)
               mosiWord = (mosiWord << 1) | bit;
            else
               mosiWord |= bit << bitCount;
         }
         if (mMiso != NULL) {
            mMiso->AdvanceToAbsPosition(sample);
            const U64 bit = mMiso->GetBitState() == BIT_HIGH ? 1 : 0;
            if (cfg.shiftOrder == AnalyzerEnums::MsbFirst)
               misoWord = (misoWord << 1) | bit;
            else
               misoWord |= bit << bitCount;
         }
         ++bitCount;
         if (bitCount == cfg.bitsPerTransfer) {
            const DecodedWord word = {firstSample, lastSample, mosiWord, misoWord};
            EmitWord(cs, word);
            mResults->CommitResults();
            ReportProgress(sample);
            bitCount = 0;
            mosiWord = 0;
            misoWord = 0;
         }
      }
      return;
   }

   // MUX_EN is active low. The enable edges define complete transactions;
   // the clock is never allowed to advance past the current rising edge.
   while (true) {
      CheckIfThreadShouldExit();
      if (!mEnable->DoMoreTransitionsExistInCurrentData())
         return;
      mEnable->AdvanceToNextEdge();
      if (mEnable->GetBitState() != BIT_LOW)
         continue;

      const U64 start = mEnable->GetSampleNumber();
      const U32 cs = ReadAddress(start);

      if (!mEnable->DoMoreTransitionsExistInCurrentData())
         return;
      const U64 end = mEnable->GetSampleOfNextEdge();
      DecodeSelection(start, end, cs);
      mEnable->AdvanceToNextEdge();
      ReportProgress(end);
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
const char *SpiAnalyzer::GetAnalyzerName() const { return "SPI MUX"; }
bool SpiAnalyzer::NeedsRerun() { return false; }
const char *GetAnalyzerName() { return "SPI MUX"; }
Analyzer *CreateAnalyzer() { return new SpiAnalyzer(); }
void DestroyAnalyzer(Analyzer *analyzer) { delete analyzer; }
