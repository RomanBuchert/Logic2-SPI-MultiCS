#ifndef SPI_ANALYZER_SETTINGS
#define SPI_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class SpiAnalyzerSettings : public AnalyzerSettings
{
  public:
    SpiAnalyzerSettings();
    virtual ~SpiAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();

    void UpdateInterfacesFromSettings();

    Channel mMosiChannel;
    Channel mMisoChannel;
    Channel mClockChannel;
    Channel mEnableChannel;
    static const U32 kClientCount = 4;
    Channel mExtraCS[3];
    AnalyzerEnums::ShiftOrder mClientShiftOrder[kClientCount];
    U32 mClientBitsPerTransfer[kClientCount];
    BitState mClientClockInactiveState[kClientCount];
    AnalyzerEnums::Edge mClientDataValidEdge[kClientCount];
    BitState mClientEnableActiveState[kClientCount];
    AnalyzerEnums::ShiftOrder mShiftOrder;
    U32 mBitsPerTransfer;
    BitState mClockInactiveState;
    AnalyzerEnums::Edge mDataValidEdge;
    BitState mEnableActiveState;


  protected:
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mMosiChannelInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mMisoChannelInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mClockChannelInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mEnableChannelInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mExtraCSInterfaces[3];
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mClientShiftOrderInterfaces[3];
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mClientBitsInterfaces[3];
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mClientClockInterfaces[3];
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mClientPhaseInterfaces[3];
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mClientPolarityInterfaces[3];
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mShiftOrderInterface;
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mBitsPerTransferInterface;
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mClockInactiveStateInterface;
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mDataValidEdgeInterface;
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mEnableActiveStateInterface;
};

#endif // SPI_ANALYZER_SETTINGS
