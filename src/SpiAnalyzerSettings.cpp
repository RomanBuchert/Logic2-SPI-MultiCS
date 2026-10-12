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
   mMuxEnableInterface->SetTitleAndTooltip("MUX_EN", "Active-low MUX enable (required)");
   mMuxEnableInterface->SetChannel(mMuxEnableChannel);
   AddInterface(mMuxEnableInterface.get());

   for (U32 i = 0; i < 4; ++i) {
      mMuxAddressChannels[i] = UNDEFINED_CHANNEL;
      mMuxAddressInterfaces[i].reset(new AnalyzerSettingInterfaceChannel());
      std::stringstream label;
      label << "MUX_" << char('A' + i);
      mMuxAddressInterfaces[i]->SetTitleAndTooltip(label.str().c_str(), "Required MUX address bit");
      mMuxAddressInterfaces[i]->SetChannel(mMuxAddressChannels[i]);
      AddInterface(mMuxAddressInterfaces[i].get());
   }

   for (U32 i = 0; i < kClientCount; ++i) {
      mClients[i].shiftOrder = AnalyzerEnums::MsbFirst;
      mClients[i].bitsPerTransfer = 8;
      mClients[i].clockIdle = BIT_LOW;
      mClients[i].phase = AnalyzerEnums::LeadingEdge;
      std::stringstream prefix;
      prefix << "CS" << i << " ";

      mShiftInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mShiftInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Bit Order").c_str(), "");
      mShiftInterfaces[i]->AddNumber(AnalyzerEnums::MsbFirst, "MSB first", "");
      mShiftInterfaces[i]->AddNumber(AnalyzerEnums::LsbFirst, "LSB first", "");
      mShiftInterfaces[i]->SetNumber(mClients[i].shiftOrder);

      mBitsInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mBitsInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Bits").c_str(), "");
      for (U32 bits = 1; bits <= 64; ++bits) {
         std::stringstream label;
         label << bits;
         mBitsInterfaces[i]->AddNumber(bits, label.str().c_str(), "");
      }
      mBitsInterfaces[i]->SetNumber(8);

      mClockIdleInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mClockIdleInterfaces[i]->SetTitleAndTooltip((prefix.str() + "CPOL").c_str(), "");
      mClockIdleInterfaces[i]->AddNumber(BIT_LOW, "CPOL 0", "");
      mClockIdleInterfaces[i]->AddNumber(BIT_HIGH, "CPOL 1", "");
      mClockIdleInterfaces[i]->SetNumber(BIT_LOW);

      mPhaseInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
      mPhaseInterfaces[i]->SetTitleAndTooltip((prefix.str() + "CPHA").c_str(), "");
      mPhaseInterfaces[i]->AddNumber(AnalyzerEnums::LeadingEdge, "CPHA 0", "");
      mPhaseInterfaces[i]->AddNumber(AnalyzerEnums::TrailingEdge, "CPHA 1", "");
      mPhaseInterfaces[i]->SetNumber(AnalyzerEnums::LeadingEdge);

      AddInterface(mShiftInterfaces[i].get());
      AddInterface(mBitsInterfaces[i].get());
      AddInterface(mClockIdleInterfaces[i].get());
      AddInterface(mPhaseInterfaces[i].get());
   }

   AddExportOption(0, "Export as text/csv file");
   AddExportExtension(0, "csv", "csv");
   RegisterChannels();
}

SpiAnalyzerSettings::~SpiAnalyzerSettings() {}

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
   if (clock == UNDEFINED_CHANNEL || enable == UNDEFINED_CHANNEL) {
      SetErrorText("SCK and MUX_EN are required.");
      return false;
   }
   for (U32 i = 0; i < 4; ++i) {
      if (mMuxAddressInterfaces[i]->GetChannel() == UNDEFINED_CHANNEL) {
         SetErrorText("All four MUX address channels A-D are required.");
         return false;
      }
   }
   if (mosi == UNDEFINED_CHANNEL && miso == UNDEFINED_CHANNEL) {
      SetErrorText("MOSI or MISO is required.");
      return false;
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
   for (U32 i = 0; i < kClientCount; ++i) {
      mClients[i].shiftOrder = (AnalyzerEnums::ShiftOrder)U32(mShiftInterfaces[i]->GetNumber());
      mClients[i].bitsPerTransfer = U32(mBitsInterfaces[i]->GetNumber());
      mClients[i].clockIdle = (BitState)U32(mClockIdleInterfaces[i]->GetNumber());
      mClients[i].phase = (AnalyzerEnums::Edge)U32(mPhaseInterfaces[i]->GetNumber());
   }
   RegisterChannels();
   return true;
}

void SpiAnalyzerSettings::LoadSettings(const char *settings)
{
   SimpleArchive archive;
   archive.SetString(settings);
   const char *name = NULL;
   archive >> &name;
   if (name == NULL || strcmp(name, "RB_SpiMuxAnalyzer_V1") != 0) {
      AnalyzerHelpers::Assert("Invalid SPI MUX settings version");
      return;
   }
   archive >> mMosiChannel;
   archive >> mMisoChannel;
   archive >> mClockChannel;
   archive >> mMuxEnableChannel;
   for (U32 i = 0; i < 4; ++i) archive >> mMuxAddressChannels[i];
   for (U32 i = 0; i < kClientCount; ++i) {
      U32 order, bits, idle, phase;
      archive >> order;
      archive >> bits;
      archive >> idle;
      archive >> phase;
      mClients[i].shiftOrder = (AnalyzerEnums::ShiftOrder)order;
      mClients[i].bitsPerTransfer = bits >= 1 && bits <= 64 ? bits : 8;
      mClients[i].clockIdle = (BitState)idle;
      mClients[i].phase = (AnalyzerEnums::Edge)phase;
   }
   RegisterChannels();
   UpdateInterfacesFromSettings();
}

const char *SpiAnalyzerSettings::SaveSettings()
{
   SimpleArchive archive;
   archive << "RB_SpiMuxAnalyzer_V1";
   archive << mMosiChannel;
   archive << mMisoChannel;
   archive << mClockChannel;
   archive << mMuxEnableChannel;
   for (U32 i = 0; i < 4; ++i) archive << mMuxAddressChannels[i];
   for (U32 i = 0; i < kClientCount; ++i) {
      archive << mClients[i].shiftOrder;
      archive << mClients[i].bitsPerTransfer;
      archive << mClients[i].clockIdle;
      archive << mClients[i].phase;
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
   for (U32 i = 0; i < kClientCount; ++i) {
      mShiftInterfaces[i]->SetNumber(mClients[i].shiftOrder);
      mBitsInterfaces[i]->SetNumber(mClients[i].bitsPerTransfer);
      mClockIdleInterfaces[i]->SetNumber(mClients[i].clockIdle);
      mPhaseInterfaces[i]->SetNumber(mClients[i].phase);
   }
}
