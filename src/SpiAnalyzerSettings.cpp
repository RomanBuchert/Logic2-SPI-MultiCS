#include "SpiAnalyzerSettings.h"
#include <AnalyzerHelpers.h>
#include <cstring>
#include <sstream>
#include <vector>

SpiAnalyzerSettings::SpiAnalyzerSettings()
   : mMosiChannel(UNDEFINED_CHANNEL), mMisoChannel(UNDEFINED_CHANNEL),
     mClockChannel(UNDEFINED_CHANNEL), mMuxEnableChannel(UNDEFINED_CHANNEL)
{
   mMosiInterface.reset(new AnalyzerSettingInterfaceChannel());
   mMosiInterface->SetTitleAndTooltip("MOSI", "Master Out, Slave In");
   mMosiInterface->SetSelectionOfNoneIsAllowed(true);
   mMosiInterface->SetChannel(mMosiChannel);
   AddInterface(mMosiInterface.get());

   mMisoInterface.reset(new AnalyzerSettingInterfaceChannel());
   mMisoInterface->SetTitleAndTooltip("MISO", "Master In, Slave Out");
   mMisoInterface->SetSelectionOfNoneIsAllowed(true);
   mMisoInterface->SetChannel(mMisoChannel);
   AddInterface(mMisoInterface.get());

   mClockInterface.reset(new AnalyzerSettingInterfaceChannel());
   mClockInterface->SetTitleAndTooltip("SCK", "Shared SPI clock");
   mClockInterface->SetChannel(mClockChannel);
   AddInterface(mClockInterface.get());

   mMuxEnableInterface.reset(new AnalyzerSettingInterfaceChannel());
   mMuxEnableInterface->SetTitleAndTooltip("MUX_EN", "Optional active-low MUX enable");
   mMuxEnableInterface->SetSelectionOfNoneIsAllowed(true);
   mMuxEnableInterface->SetChannel(mMuxEnableChannel);
   AddInterface(mMuxEnableInterface.get());

   for (U32 i = 0; i < 4; ++i) {
      mMuxAddressChannels[i] = UNDEFINED_CHANNEL;
      mMuxAddressInterfaces[i].reset(new AnalyzerSettingInterfaceChannel());
      std::stringstream label;
      label << "MUX_" << char('A' + i);
      mMuxAddressInterfaces[i]->SetTitleAndTooltip(label.str().c_str(), "Optional consecutive MUX address bit");
      mMuxAddressInterfaces[i]->SetSelectionOfNoneIsAllowed(true);
      mMuxAddressInterfaces[i]->SetChannel(mMuxAddressChannels[i]);
      AddInterface(mMuxAddressInterfaces[i].get());
   }


   mDefaults.shiftOrder = AnalyzerEnums::MsbFirst;
   mDefaults.bitsPerTransfer = 8;
   mDefaults.clockIdle = BIT_LOW;
   mDefaults.phase = AnalyzerEnums::LeadingEdge;

   mDefaultShiftInterface.reset(new AnalyzerSettingInterfaceNumberList());
   mDefaultShiftInterface->SetTitleAndTooltip("Default Bit Order", "Used by clients set to Default");
   mDefaultShiftInterface->AddNumber(AnalyzerEnums::MsbFirst, "MSB first", "");
   mDefaultShiftInterface->AddNumber(AnalyzerEnums::LsbFirst, "LSB first", "");
   mDefaultShiftInterface->SetNumber(mDefaults.shiftOrder);
   AddInterface(mDefaultShiftInterface.get());

   mDefaultBitsInterface.reset(new AnalyzerSettingInterfaceNumberList());
   mDefaultBitsInterface->SetTitleAndTooltip("Default Bits", "Bits per transfer");
   for (U32 bits = 1; bits <= 64; ++bits) {
      std::stringstream label;
      label << bits;
      mDefaultBitsInterface->AddNumber(bits, label.str().c_str(), "");
   }
   mDefaultBitsInterface->SetNumber(mDefaults.bitsPerTransfer);
   AddInterface(mDefaultBitsInterface.get());

   mDefaultIdleInterface.reset(new AnalyzerSettingInterfaceNumberList());
   mDefaultIdleInterface->SetTitleAndTooltip("Default Clock State (CPOL)", "");
   mDefaultIdleInterface->AddNumber(BIT_LOW, "Clock is Low when inactive (CPOL = 0)", "");
   mDefaultIdleInterface->AddNumber(BIT_HIGH, "Clock is High when inactive (CPOL = 1)", "");
   mDefaultIdleInterface->SetNumber(mDefaults.clockIdle);
   AddInterface(mDefaultIdleInterface.get());

   mDefaultPhaseInterface.reset(new AnalyzerSettingInterfaceNumberList());
   mDefaultPhaseInterface->SetTitleAndTooltip("Default Clock Phase (CPHA)", "");
   mDefaultPhaseInterface->AddNumber(AnalyzerEnums::LeadingEdge, "Data is Valid on Clock Leading Edge (CPHA = 0)", "");
   mDefaultPhaseInterface->AddNumber(AnalyzerEnums::TrailingEdge, "Data is Valid on Clock Trailing Edge (CPHA = 1)", "");
   mDefaultPhaseInterface->SetNumber(mDefaults.phase);
   AddInterface(mDefaultPhaseInterface.get());

   for (U32 i = 0; i < kClientCount; ++i) {
      mOverrides[i].shiftOrder = (AnalyzerEnums::ShiftOrder)-1;
      mOverrides[i].bitsPerTransfer = 0;
      mOverrides[i].clockIdle = (BitState)-1;
      mOverrides[i].phase = (AnalyzerEnums::Edge)-1;
      std::stringstream prefix;
      prefix << "CS" << i << " ";

      mShiftInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mShiftInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Bit Order").c_str(), "");
      mShiftInterfaces[i]->AddNumber(-1, "Default", "Use global bit order");
      mShiftInterfaces[i]->AddNumber(AnalyzerEnums::MsbFirst, "MSB first", "");
      mShiftInterfaces[i]->AddNumber(AnalyzerEnums::LsbFirst, "LSB first", "");
      mShiftInterfaces[i]->SetNumber(-1);

      mBitsInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mBitsInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Bits").c_str(), "");
      mBitsInterfaces[i]->AddNumber(0, "Default", "Use global bits per transfer");
      for (U32 bits = 1; bits <= 64; ++bits) {
         std::stringstream label;
         label << bits;
         mBitsInterfaces[i]->AddNumber(bits, label.str().c_str(), "");
      }
      mBitsInterfaces[i]->SetNumber(0);

      mClockIdleInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mClockIdleInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Clock State (CPOL)").c_str(), "");
      mClockIdleInterfaces[i]->AddNumber(-1, "Default", "Use global CPOL");
      mClockIdleInterfaces[i]->AddNumber(BIT_LOW, "Clock is Low when inactive (CPOL = 0)", "");
      mClockIdleInterfaces[i]->AddNumber(BIT_HIGH, "Clock is High when inactive (CPOL = 1)", "");
      mClockIdleInterfaces[i]->SetNumber(-1);

      mPhaseInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mPhaseInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Clock Phase (CPHA)").c_str(), "");
      mPhaseInterfaces[i]->AddNumber(-1, "Default", "Use global CPHA");
      mPhaseInterfaces[i]->AddNumber(AnalyzerEnums::LeadingEdge, "Data is Valid on Clock Leading Edge (CPHA = 0)", "");
      mPhaseInterfaces[i]->AddNumber(AnalyzerEnums::TrailingEdge, "Data is Valid on Clock Trailing Edge (CPHA = 1)", "");
      mPhaseInterfaces[i]->SetNumber(-1);

      mNameInterfaces[i].reset(new AnalyzerSettingInterfaceText());
      mNameInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Name").c_str(), "Optional device name");
      mNameInterfaces[i]->SetText("");
      AddInterface(mNameInterfaces[i].get());
      AddInterface(mShiftInterfaces[i].get());
      AddInterface(mBitsInterfaces[i].get());
      AddInterface(mClockIdleInterfaces[i].get());
      AddInterface(mPhaseInterfaces[i].get());
   }

   AddExportOption(0, "Export as text/csv file");
   AddExportExtension(0, "csv", "csv");
   ResolveClients();
   RegisterChannels();
}

SpiAnalyzerSettings::~SpiAnalyzerSettings() {}

void SpiAnalyzerSettings::ResolveClients()
{
   for (U32 i = 0; i < kClientCount; ++i) {
      mClients[i].shiftOrder = (int)mOverrides[i].shiftOrder < 0
                                  ? mDefaults.shiftOrder : mOverrides[i].shiftOrder;
      mClients[i].bitsPerTransfer = mOverrides[i].bitsPerTransfer == 0
                                      ? mDefaults.bitsPerTransfer : mOverrides[i].bitsPerTransfer;
      mClients[i].clockIdle = (int)mOverrides[i].clockIdle < 0
                                 ? mDefaults.clockIdle : mOverrides[i].clockIdle;
      mClients[i].phase = (int)mOverrides[i].phase < 0
                             ? mDefaults.phase : mOverrides[i].phase;
   }
}

std::string SpiAnalyzerSettings::GetClientLabel(U32 cs) const
{
   std::stringstream label;
   label << "CS";
   if (cs < 10)
      label << '0';
   label << cs;
   if (cs < kClientCount && !mClientNames[cs].empty())
      label << ':' << mClientNames[cs];
   return label.str();
}

void SpiAnalyzerSettings::RegisterChannels()
{
   ClearChannels();
   AddChannel(mMosiChannel, "MOSI", mMosiChannel != UNDEFINED_CHANNEL);
   AddChannel(mMisoChannel, "MISO", mMisoChannel != UNDEFINED_CHANNEL);
   AddChannel(mClockChannel, "SCK", mClockChannel != UNDEFINED_CHANNEL);
   AddChannel(mMuxEnableChannel, "MUX_EN", mMuxEnableChannel != UNDEFINED_CHANNEL);
   for (U32 i = 0; i < 4; ++i) {
      std::stringstream label;
      label << "MUX_" << char('A' + i);
      AddChannel(mMuxAddressChannels[i], label.str().c_str(),
                 mMuxAddressChannels[i] != UNDEFINED_CHANNEL);
   }
}

bool SpiAnalyzerSettings::SetSettingsFromInterfaces()
{
   Channel mosi = mMosiInterface->GetChannel();
   Channel miso = mMisoInterface->GetChannel();
   Channel clock = mClockInterface->GetChannel();
   Channel enable = mMuxEnableInterface->GetChannel();
   std::vector<Channel> channels;
   channels.push_back(mosi);
   channels.push_back(miso);
   channels.push_back(clock);
   channels.push_back(enable);
   for (U32 i = 0; i < 4; ++i)
      channels.push_back(mMuxAddressInterfaces[i]->GetChannel());
   if (clock == UNDEFINED_CHANNEL) {
      SetErrorText("SCK is required.");
      return false;
   }
   for (U32 i = 1; i < 4; ++i) {
      if (mMuxAddressInterfaces[i]->GetChannel() != UNDEFINED_CHANNEL &&
          mMuxAddressInterfaces[i - 1]->GetChannel() == UNDEFINED_CHANNEL) {
         SetErrorText("MUX address pins must be consecutive starting with MUX_A.");
         return false;
      }
   }
   if (AnalyzerHelpers::DoChannelsOverlap(&channels[0], U32(channels.size()))) {
      SetErrorText("Each input must use a different channel.");
      return false;
   }
   mMosiChannel = mosi;
   mMisoChannel = miso;
   mClockChannel = clock;
   mMuxEnableChannel = enable;
   for (U32 i = 0; i < 4; ++i)
      mMuxAddressChannels[i] = mMuxAddressInterfaces[i]->GetChannel();
   mDefaults.shiftOrder = (AnalyzerEnums::ShiftOrder)(int)mDefaultShiftInterface->GetNumber();
   mDefaults.bitsPerTransfer = U32(mDefaultBitsInterface->GetNumber());
   mDefaults.clockIdle = (BitState)(int)mDefaultIdleInterface->GetNumber();
   mDefaults.phase = (AnalyzerEnums::Edge)(int)mDefaultPhaseInterface->GetNumber();
   for (U32 i = 0; i < kClientCount; ++i) {
      mOverrides[i].shiftOrder = (AnalyzerEnums::ShiftOrder)(int)mShiftInterfaces[i]->GetNumber();
      mOverrides[i].bitsPerTransfer = U32(mBitsInterfaces[i]->GetNumber());
      mOverrides[i].clockIdle = (BitState)(int)mClockIdleInterfaces[i]->GetNumber();
      mOverrides[i].phase = (AnalyzerEnums::Edge)(int)mPhaseInterfaces[i]->GetNumber();
      const char *name = mNameInterfaces[i]->GetText();
      mClientNames[i] = name == NULL ? "" : name;
   }
   ResolveClients();
   RegisterChannels();
   return true;
}

void SpiAnalyzerSettings::LoadSettings(const char *settings)
{
   SimpleArchive archive;
   archive.SetString(settings);
   const char *version = NULL;
   archive >> &version;
   if (version == NULL || (strcmp(version, "RB_SpiMuxAnalyzer_V1") != 0 &&
                           strcmp(version, "RB_SpiMuxAnalyzer_V2") != 0)) {
      AnalyzerHelpers::Assert("Invalid SPI MUX settings version");
      return;
   }
   const bool legacy = strcmp(version, "RB_SpiMuxAnalyzer_V1") == 0;
   archive >> mMosiChannel;
   archive >> mMisoChannel;
   archive >> mClockChannel;
   archive >> mMuxEnableChannel;
   for (U32 i = 0; i < 4; ++i)
      archive >> mMuxAddressChannels[i];
   if (!legacy) {
      U32 order, bits, idle, phase;
      archive >> order;
      archive >> bits;
      archive >> idle;
      archive >> phase;
      mDefaults.shiftOrder = (AnalyzerEnums::ShiftOrder)order;
      mDefaults.bitsPerTransfer = bits >= 1 && bits <= 64 ? bits : 8;
      mDefaults.clockIdle = (BitState)idle;
      mDefaults.phase = (AnalyzerEnums::Edge)phase;
   }
   for (U32 i = 0; i < kClientCount; ++i) {
      U32 order, bits, idle, phase;
      archive >> order;
      archive >> bits;
      archive >> idle;
      archive >> phase;
      mOverrides[i].shiftOrder = legacy ? (AnalyzerEnums::ShiftOrder)order
                                         : (AnalyzerEnums::ShiftOrder)(int)order;
      mOverrides[i].bitsPerTransfer = legacy && (bits < 1 || bits > 64) ? 8 : bits;
      mOverrides[i].clockIdle = (BitState)(int)idle;
      mOverrides[i].phase = (AnalyzerEnums::Edge)(int)phase;
      if (!legacy) {
         const char *name = NULL;
         archive >> &name;
         mClientNames[i] = name == NULL ? "" : name;
      } else {
         mClientNames[i].clear();
      }
   }
   ResolveClients();
   RegisterChannels();
   UpdateInterfacesFromSettings();
}

const char *SpiAnalyzerSettings::SaveSettings()
{
   SimpleArchive archive;
   archive << "RB_SpiMuxAnalyzer_V2";
   archive << mMosiChannel;
   archive << mMisoChannel;
   archive << mClockChannel;
   archive << mMuxEnableChannel;
   for (U32 i = 0; i < 4; ++i)
      archive << mMuxAddressChannels[i];
   archive << U32(mDefaults.shiftOrder);
   archive << mDefaults.bitsPerTransfer;
   archive << U32(mDefaults.clockIdle);
   archive << U32(mDefaults.phase);
   for (U32 i = 0; i < kClientCount; ++i) {
      archive << U32(mOverrides[i].shiftOrder);
      archive << mOverrides[i].bitsPerTransfer;
      archive << U32(mOverrides[i].clockIdle);
      archive << U32(mOverrides[i].phase);
      archive << mClientNames[i].c_str();
   }
   return SetReturnString(archive.GetString());
}

void SpiAnalyzerSettings::UpdateInterfacesFromSettings()
{
   mMosiInterface->SetChannel(mMosiChannel);
   mMisoInterface->SetChannel(mMisoChannel);
   mClockInterface->SetChannel(mClockChannel);
   mMuxEnableInterface->SetChannel(mMuxEnableChannel);
   for (U32 i = 0; i < 4; ++i)
      mMuxAddressInterfaces[i]->SetChannel(mMuxAddressChannels[i]);
   mDefaultShiftInterface->SetNumber(mDefaults.shiftOrder);
   mDefaultBitsInterface->SetNumber(mDefaults.bitsPerTransfer);
   mDefaultIdleInterface->SetNumber(mDefaults.clockIdle);
   mDefaultPhaseInterface->SetNumber(mDefaults.phase);
   for (U32 i = 0; i < kClientCount; ++i) {
      mNameInterfaces[i]->SetText(mClientNames[i].c_str());
      mShiftInterfaces[i]->SetNumber(mOverrides[i].shiftOrder);
      mBitsInterfaces[i]->SetNumber(mOverrides[i].bitsPerTransfer);
      mClockIdleInterfaces[i]->SetNumber(mOverrides[i].clockIdle);
      mPhaseInterfaces[i]->SetNumber(mOverrides[i].phase);
   }
}
