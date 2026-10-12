#ifndef SPI_ANALYZER_SETTINGS
#define SPI_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>
#include <memory>

struct SpiClientSettings
{
   AnalyzerEnums::ShiftOrder shiftOrder;
   U32 bitsPerTransfer;
   BitState clockIdle;
   AnalyzerEnums::Edge phase;
};

class SpiAnalyzerSettings : public AnalyzerSettings
{
 public:
   static const U32 kClientCount = 16;
   SpiAnalyzerSettings();
   virtual ~SpiAnalyzerSettings();
   virtual bool SetSettingsFromInterfaces();
   virtual void LoadSettings(const char *settings);
   virtual const char *SaveSettings();
   void UpdateInterfacesFromSettings();

   Channel mMosiChannel;
   Channel mMisoChannel;
   Channel mClockChannel;
   Channel mMuxEnableChannel;
   Channel mMuxAddressChannels[4];
   SpiClientSettings mClients[kClientCount];

 private:
   std::auto_ptr<AnalyzerSettingInterfaceChannel> mMosiInterface;
   std::auto_ptr<AnalyzerSettingInterfaceChannel> mMisoInterface;
   std::auto_ptr<AnalyzerSettingInterfaceChannel> mClockInterface;
   std::auto_ptr<AnalyzerSettingInterfaceChannel> mMuxEnableInterface;
   std::auto_ptr<AnalyzerSettingInterfaceChannel> mMuxAddressInterfaces[4];
   std::auto_ptr<AnalyzerSettingInterfaceNumberList> mShiftInterfaces[kClientCount];
   std::auto_ptr<AnalyzerSettingInterfaceNumberList> mBitsInterfaces[kClientCount];
   std::auto_ptr<AnalyzerSettingInterfaceNumberList> mClockIdleInterfaces[kClientCount];
   std::auto_ptr<AnalyzerSettingInterfaceNumberList> mPhaseInterfaces[kClientCount];
   void RegisterChannels();
};
#endif
