#include <Analyzer.h>
#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>
#include <AnalyzerResults.h>
#include <AnalyzerSettings.h>
#include <memory>
#include <cstdio>
#include <cstring>

class MuxSelectionSettings : public AnalyzerSettings
{
 public:
   MuxSelectionSettings() : mEnable(UNDEFINED_CHANNEL)
   {
      mEnableInterface.reset(new AnalyzerSettingInterfaceChannel());
      mEnableInterface->SetTitleAndTooltip("MUX_EN", "Active-low MUX enable");
      mEnableInterface->SetChannel(mEnable);
      AddInterface(mEnableInterface.get());

      for (U32 i = 0; i < 4; ++i) {
         mAddress[i] = UNDEFINED_CHANNEL;
         mAddressInterfaces[i].reset(new AnalyzerSettingInterfaceChannel());
         char title[16];
         std::sprintf(title, "MUX_%c", char('A' + i));
         mAddressInterfaces[i]->SetTitleAndTooltip(title, "MUX address bit");
         mAddressInterfaces[i]->SetChannel(mAddress[i]);
         AddInterface(mAddressInterfaces[i].get());
      }
      RegisterChannels();
   }

   bool SetSettingsFromInterfaces()
   {
      Channel channels[5];
      channels[0] = mEnableInterface->GetChannel();
      for (U32 i = 0; i < 4; ++i)
         channels[i + 1] = mAddressInterfaces[i]->GetChannel();
      for (U32 i = 0; i < 5; ++i) {
         if (channels[i] == UNDEFINED_CHANNEL) {
            SetErrorText("MUX_EN and all four MUX address channels are required.");
            return false;
         }
      }
      if (AnalyzerHelpers::DoChannelsOverlap(channels, 5)) {
         SetErrorText("Each MUX input must use a different channel.");
         return false;
      }
      mEnable = channels[0];
      for (U32 i = 0; i < 4; ++i)
         mAddress[i] = channels[i + 1];
      RegisterChannels();
      return true;
   }

   void LoadSettings(const char *settings)
   {
      SimpleArchive archive;
      archive.SetString(settings);
      const char *version = NULL;
      archive >> &version;
      if (version == NULL || std::strcmp(version, "RB_MuxSelection_V1") != 0)
         return;
      archive >> mEnable;
      for (U32 i = 0; i < 4; ++i)
         archive >> mAddress[i];
      RegisterChannels();
      UpdateInterfacesFromSettings();
   }

   const char *SaveSettings()
   {
      SimpleArchive archive;
      archive << "RB_MuxSelection_V1";
      archive << mEnable;
      for (U32 i = 0; i < 4; ++i)
         archive << mAddress[i];
      return SetReturnString(archive.GetString());
   }

   void UpdateInterfacesFromSettings()
   {
      mEnableInterface->SetChannel(mEnable);
      for (U32 i = 0; i < 4; ++i)
         mAddressInterfaces[i]->SetChannel(mAddress[i]);
   }

   void RegisterChannels()
   {
      ClearChannels();
      AddChannel(mEnable, "MUX_EN", mEnable != UNDEFINED_CHANNEL);
      for (U32 i = 0; i < 4; ++i) {
         char title[16];
         std::sprintf(title, "MUX_%c", char('A' + i));
         AddChannel(mAddress[i], title, mAddress[i] != UNDEFINED_CHANNEL);
      }
   }

   Channel mEnable;
   Channel mAddress[4];

 private:
   std::auto_ptr<AnalyzerSettingInterfaceChannel> mEnableInterface;
   std::auto_ptr<AnalyzerSettingInterfaceChannel> mAddressInterfaces[4];
};

class MuxSelectionResults : public AnalyzerResults
{
 public:
   explicit MuxSelectionResults(MuxSelectionSettings *settings) : mSettings(settings) {}

   void GenerateBubbleText(U64 index, Channel &channel, DisplayBase)
   {
      ClearResultStrings();
      if (channel != mSettings->mEnable)
         return;
      Frame frame = GetFrame(index);
      char text[16];
      std::sprintf(text, "CS%02u", U32(frame.mData1));
      AddResultString(text);
   }

   void GenerateFrameTabularText(U64 index, DisplayBase)
   {
      ClearTabularText();
      Frame frame = GetFrame(index);
      char text[16];
      std::sprintf(text, "CS%02u", U32(frame.mData1));
      AddTabularText(text);
   }

   void GenerateExportFile(const char *, DisplayBase, U32) {}
   void GeneratePacketTabularText(U64, DisplayBase) {}
   void GenerateTransactionTabularText(U64, DisplayBase) {}

 private:
   MuxSelectionSettings *mSettings;
};

class MuxSelectionAnalyzer : public Analyzer2
{
 public:
   MuxSelectionAnalyzer() : mSettings(new MuxSelectionSettings())
   {
      SetAnalyzerSettings(mSettings.get());
   }

   ~MuxSelectionAnalyzer() { KillThread(); }

   void SetupResults()
   {
      mResults.reset(new MuxSelectionResults(mSettings.get()));
      SetAnalyzerResults(mResults.get());
      mResults->AddChannelBubblesWillAppearOn(mSettings->mEnable);
   }

   void WorkerThread()
   {
      AnalyzerChannelData *enable = GetAnalyzerChannelData(mSettings->mEnable);
      AnalyzerChannelData *address[4];
      for (U32 i = 0; i < 4; ++i)
         address[i] = GetAnalyzerChannelData(mSettings->mAddress[i]);

      while (true) {
         CheckIfThreadShouldExit();
         if (!enable->DoMoreTransitionsExistInCurrentData())
            return;
         enable->AdvanceToNextEdge();
         if (enable->GetBitState() != BIT_LOW)
            continue;

         const U64 start = enable->GetSampleNumber();
         U32 cs = 0;
         for (U32 i = 0; i < 4; ++i) {
            address[i]->AdvanceToAbsPosition(start);
            if (address[i]->GetBitState() == BIT_HIGH)
               cs |= 1U << i;
         }

         if (!enable->DoMoreTransitionsExistInCurrentData())
            return;
         const U64 end = enable->GetSampleOfNextEdge();

         Frame frame = {};
         frame.mStartingSampleInclusive = start;
         frame.mEndingSampleInclusive = end > start ? end - 1 : start;
         frame.mData1 = cs;
         mResults->AddFrame(frame);
         mResults->CommitResults();

         enable->AdvanceToNextEdge();
         ReportProgress(end);
      }
   }

   U32 GenerateSimulationData(U64, U32, SimulationChannelDescriptor **channels)
   {
      *channels = NULL;
      return 0;
   }

   U32 GetMinimumSampleRateHz() { return 10000; }
   const char *GetAnalyzerName() const { return "MUX Selection (RB)"; }
   bool NeedsRerun() { return false; }

 private:
   std::auto_ptr<MuxSelectionSettings> mSettings;
   std::auto_ptr<MuxSelectionResults> mResults;
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName()
{
   return "MUX Selection (RB)";
}

extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer()
{
   return new MuxSelectionAnalyzer();
}

extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer)
{
   delete analyzer;
}
