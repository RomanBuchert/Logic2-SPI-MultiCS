#include "SpiAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

SpiAnalyzerSettings::SpiAnalyzerSettings()
    : mMosiChannel( UNDEFINED_CHANNEL ),
      mMisoChannel( UNDEFINED_CHANNEL ),
      mClockChannel( UNDEFINED_CHANNEL ),
      mEnableChannel( UNDEFINED_CHANNEL ),
      mShiftOrder( AnalyzerEnums::MsbFirst ),
      mBitsPerTransfer( 8 ),
      mClockInactiveState( BIT_LOW ),
      mDataValidEdge( AnalyzerEnums::LeadingEdge ),
      mEnableActiveState( BIT_LOW )
{
    mMosiChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mMosiChannelInterface->SetTitleAndTooltip( "MOSI", "Master Out, Slave In" );
    mMosiChannelInterface->SetChannel( mMosiChannel );
    mMosiChannelInterface->SetSelectionOfNoneIsAllowed( true );

    mMisoChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mMisoChannelInterface->SetTitleAndTooltip( "MISO", "Master In, Slave Out" );
    mMisoChannelInterface->SetChannel( mMisoChannel );
    mMisoChannelInterface->SetSelectionOfNoneIsAllowed( true );

    mClockChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mClockChannelInterface->SetTitleAndTooltip( "Clock", "Clock (CLK)" );
    mClockChannelInterface->SetChannel( mClockChannel );

    mEnableChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mEnableChannelInterface->SetTitleAndTooltip( "Enable", "Enable (SS, Slave Select)" );
    mEnableChannelInterface->SetChannel( mEnableChannel );
    for (U32 i = 0; i < 3; ++i) mExtraCSInterfaces[i]->SetChannel(mExtraCS[i]);
    mEnableChannelInterface->SetSelectionOfNoneIsAllowed( true );

    for (U32 i = 0; i < 3; ++i) {
        mExtraCS[i] = UNDEFINED_CHANNEL;
        mExtraCSInterfaces[i].reset(new AnalyzerSettingInterfaceChannel());
        std::stringstream title;
        title << "CS" << (i + 1);
        mExtraCSInterfaces[i]->SetTitleAndTooltip(title.str().c_str(), "Additional chip select");
        mExtraCSInterfaces[i]->SetChannel(UNDEFINED_CHANNEL);
        mExtraCSInterfaces[i]->SetSelectionOfNoneIsAllowed(true);
    }
    for (U32 i = 0; i < kClientCount; ++i) {
        mClientShiftOrder[i] = AnalyzerEnums::MsbFirst;
        mClientBitsPerTransfer[i] = 8;
        mClientClockInactiveState[i] = BIT_LOW;
        mClientDataValidEdge[i] = AnalyzerEnums::LeadingEdge;
        mClientEnableActiveState[i] = BIT_LOW;
    }
    for (U32 i = 0; i < 3; ++i) {
        std::stringstream prefix;
        prefix << "CS" << (i + 1) << " ";
        mClientShiftOrderInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
        mClientShiftOrderInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Bit Order").c_str(), "");
        mClientShiftOrderInterfaces[i]->AddNumber(AnalyzerEnums::MsbFirst, "MSB first", "");
        mClientShiftOrderInterfaces[i]->AddNumber(AnalyzerEnums::LsbFirst, "LSB first", "");
        mClientBitsInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
        mClientBitsInterfaces[i]->SetTitleAndTooltip((prefix.str() + "Bits per Transfer").c_str(), "");
        for (U32 bits = 1; bits <= 64; ++bits) {
            std::stringstream label;
            label << bits << " bits";
            mClientBitsInterfaces[i]->AddNumber(bits, label.str().c_str(), "");
        }
        mClientClockInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
        mClientClockInterfaces[i]->SetTitleAndTooltip((prefix.str() + "CPOL").c_str(), "");
        mClientClockInterfaces[i]->AddNumber(BIT_LOW, "CPOL 0", "");
        mClientClockInterfaces[i]->AddNumber(BIT_HIGH, "CPOL 1", "");
        mClientPhaseInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
        mClientPhaseInterfaces[i]->SetTitleAndTooltip((prefix.str() + "CPHA").c_str(), "");
        mClientPhaseInterfaces[i]->AddNumber(AnalyzerEnums::LeadingEdge, "CPHA 0", "");
        mClientPhaseInterfaces[i]->AddNumber(AnalyzerEnums::TrailingEdge, "CPHA 1", "");
        mClientPolarityInterfaces[i].reset(new AnalyzerSettingInterfaceNumberList());
        mClientPolarityInterfaces[i]->SetTitleAndTooltip((prefix.str() + "CS Polarity").c_str(), "");
        mClientPolarityInterfaces[i]->AddNumber(BIT_LOW, "Active low", "");
        mClientPolarityInterfaces[i]->AddNumber(BIT_HIGH, "Active high", "");
        mClientShiftOrderInterfaces[i]->SetNumber(mClientShiftOrder[i + 1]);
        mClientBitsInterfaces[i]->SetNumber(mClientBitsPerTransfer[i + 1]);
        mClientClockInterfaces[i]->SetNumber(mClientClockInactiveState[i + 1]);
        mClientPhaseInterfaces[i]->SetNumber(mClientDataValidEdge[i + 1]);
        mClientPolarityInterfaces[i]->SetNumber(mClientEnableActiveState[i + 1]);
    }
    mShiftOrderInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mShiftOrderInterface->SetTitleAndTooltip( "Significant Bit", "" );
    mShiftOrderInterface->AddNumber( AnalyzerEnums::MsbFirst, "Most Significant Bit First (Standard)",
                                     "Select if the most significant bit or least significant bit is transmitted first" );
    mShiftOrderInterface->AddNumber( AnalyzerEnums::LsbFirst, "Least Significant Bit First", "" );
    mShiftOrderInterface->SetNumber( mShiftOrder );

    mBitsPerTransferInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mBitsPerTransferInterface->SetTitleAndTooltip( "Bits per Transfer", "" );
    for( U32 i = 1; i <= 64; i++ )
    {
        std::stringstream ss;

        if( i == 8 )
            ss << "8 Bits per Transfer (Standard)";
        else
            ss << i << " Bits per Transfer";

        mBitsPerTransferInterface->AddNumber( i, ss.str().c_str(), "" );
    }
    mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );

    mClockInactiveStateInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mClockInactiveStateInterface->SetTitleAndTooltip( "Clock State", "" );
    mClockInactiveStateInterface->AddNumber( BIT_LOW, "Clock is Low when inactive (CPOL = 0)", "CPOL = 0 (Clock Polarity)" );
    mClockInactiveStateInterface->AddNumber( BIT_HIGH, "Clock is High when inactive (CPOL = 1)", "CPOL = 1 (Clock Polarity)" );
    mClockInactiveStateInterface->SetNumber( mClockInactiveState );

    mDataValidEdgeInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mDataValidEdgeInterface->SetTitleAndTooltip( "Clock Phase", "" );
    mDataValidEdgeInterface->AddNumber( AnalyzerEnums::LeadingEdge, "Data is Valid on Clock Leading Edge (CPHA = 0)",
                                        "CPHA = 0 (Clock Phase)" );
    mDataValidEdgeInterface->AddNumber( AnalyzerEnums::TrailingEdge, "Data is Valid on Clock Trailing Edge (CPHA = 1)",
                                        "CPHA = 1 (Clock Phase)" );
    mDataValidEdgeInterface->SetNumber( mDataValidEdge );

    mEnableActiveStateInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mEnableActiveStateInterface->SetTitleAndTooltip( "Enable Line", "" );
    mEnableActiveStateInterface->AddNumber( BIT_LOW, "Enable line is Active Low (Standard)", "" );
    mEnableActiveStateInterface->AddNumber( BIT_HIGH, "Enable line is Active High", "" );
    mEnableActiveStateInterface->SetNumber( mEnableActiveState );


    AddInterface( mMosiChannelInterface.get() );
    AddInterface( mMisoChannelInterface.get() );
    AddInterface( mClockChannelInterface.get() );
    AddInterface( mEnableChannelInterface.get() );
    for (U32 i = 0; i < 3; ++i) AddInterface(mExtraCSInterfaces[i].get());
    for (U32 i = 0; i < 3; ++i) {
        AddInterface(mClientShiftOrderInterfaces[i].get());
        AddInterface(mClientBitsInterfaces[i].get());
        AddInterface(mClientClockInterfaces[i].get());
        AddInterface(mClientPhaseInterfaces[i].get());
        AddInterface(mClientPolarityInterfaces[i].get());
    }
    AddInterface( mShiftOrderInterface.get() );
    AddInterface( mBitsPerTransferInterface.get() );
    AddInterface( mClockInactiveStateInterface.get() );
    AddInterface( mDataValidEdgeInterface.get() );
    AddInterface( mEnableActiveStateInterface.get() );


    // AddExportOption( 0, "Export as text/csv file", "text (*.txt);;csv (*.csv)" );
    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mMosiChannel, "MOSI", false );
    AddChannel( mMisoChannel, "MISO", false );
    AddChannel( mClockChannel, "CLOCK", false );
    AddChannel( mEnableChannel, "ENABLE", false );
}

SpiAnalyzerSettings::~SpiAnalyzerSettings()
{
}

bool SpiAnalyzerSettings::SetSettingsFromInterfaces()
{
    Channel mosi = mMosiChannelInterface->GetChannel();
    Channel miso = mMisoChannelInterface->GetChannel();
    Channel clock = mClockChannelInterface->GetChannel();
    Channel enable = mEnableChannelInterface->GetChannel();

    std::vector<Channel> channels;
    channels.push_back( mosi );
    channels.push_back( miso );
    channels.push_back( clock );
    channels.push_back( enable );
    for (U32 i = 0; i < 3; ++i) channels.push_back(mExtraCSInterfaces[i]->GetChannel());

    if( AnalyzerHelpers::DoChannelsOverlap( &channels[ 0 ], channels.size() ) == true )
    {
        SetErrorText( "Please select different channels for each input." );
        return false;
    }

    if( ( mosi == UNDEFINED_CHANNEL ) && ( miso == UNDEFINED_CHANNEL ) )
    {
        SetErrorText( "Please select at least one input for either MISO or MOSI." );
        return false;
    }

    mMosiChannel = mMosiChannelInterface->GetChannel();
    mMisoChannel = mMisoChannelInterface->GetChannel();
    mClockChannel = mClockChannelInterface->GetChannel();
    mEnableChannel = mEnableChannelInterface->GetChannel();

    for (U32 i = 0; i < 3; ++i) mExtraCS[i] = mExtraCSInterfaces[i]->GetChannel();
    for (U32 i = 0; i < 3; ++i) {
        mClientShiftOrder[i + 1] = (AnalyzerEnums::ShiftOrder)U32(mClientShiftOrderInterfaces[i]->GetNumber());
        mClientBitsPerTransfer[i + 1] = U32(mClientBitsInterfaces[i]->GetNumber());
        mClientClockInactiveState[i + 1] = (BitState)U32(mClientClockInterfaces[i]->GetNumber());
        mClientDataValidEdge[i + 1] = (AnalyzerEnums::Edge)U32(mClientPhaseInterfaces[i]->GetNumber());
        mClientEnableActiveState[i + 1] = (BitState)U32(mClientPolarityInterfaces[i]->GetNumber());
    }
    mShiftOrder = ( AnalyzerEnums::ShiftOrder )U32( mShiftOrderInterface->GetNumber() );
    mBitsPerTransfer = U32( mBitsPerTransferInterface->GetNumber() );
    mClockInactiveState = ( BitState )U32( mClockInactiveStateInterface->GetNumber() );
    mDataValidEdge = ( AnalyzerEnums::Edge )U32( mDataValidEdgeInterface->GetNumber() );
    mEnableActiveState = ( BitState )U32( mEnableActiveStateInterface->GetNumber() );

    mClientShiftOrder[0] = mShiftOrder;
    mClientBitsPerTransfer[0] = mBitsPerTransfer;
    mClientClockInactiveState[0] = mClockInactiveState;
    mClientDataValidEdge[0] = mDataValidEdge;
    mClientEnableActiveState[0] = mEnableActiveState;
    ClearChannels();
    AddChannel( mMosiChannel, "MOSI", mMosiChannel != UNDEFINED_CHANNEL );
    AddChannel( mMisoChannel, "MISO", mMisoChannel != UNDEFINED_CHANNEL );
    AddChannel( mClockChannel, "CLOCK", mClockChannel != UNDEFINED_CHANNEL );
    AddChannel( mEnableChannel, "ENABLE", mEnableChannel != UNDEFINED_CHANNEL );
    for (U32 i = 0; i < 3; ++i) AddChannel(mExtraCS[i], "CS", mExtraCS[i] != UNDEFINED_CHANNEL);

    return true;
}

void SpiAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    const char* name_string; // the first thing in the archive is the name of the protocol analyzer that the data belongs to.
    text_archive >> &name_string;
    if( strcmp( name_string, "SaleaeSpiMultiCSAnalyzer" ) != 0 && strcmp( name_string, "SaleaeSpiAnalyzer" ) != 0 )
        AnalyzerHelpers::Assert( "SaleaeSpiAnalyzer: Provided with a settings string that doesn't belong to us;" );

    text_archive >> mMosiChannel;
    text_archive >> mMisoChannel;
    text_archive >> mClockChannel;
    text_archive >> mEnableChannel;
    text_archive >> *( U32* )&mShiftOrder;
    text_archive >> mBitsPerTransfer;
    text_archive >> *( U32* )&mClockInactiveState;
    text_archive >> *( U32* )&mDataValidEdge;
    text_archive >> *( U32* )&mEnableActiveState;

    for (U32 i = 0; i < 3; ++i) {
        mExtraCS[i] = UNDEFINED_CHANNEL;
        Channel channel;
        if (text_archive >> channel) mExtraCS[i] = channel;
    }
    for (U32 i = 1; i < kClientCount; ++i) {
        U32 value;
        if (text_archive >> value) mClientShiftOrder[i] = (AnalyzerEnums::ShiftOrder)value;
        if (text_archive >> value) mClientBitsPerTransfer[i] = value;
        if (text_archive >> value) mClientClockInactiveState[i] = (BitState)value;
        if (text_archive >> value) mClientDataValidEdge[i] = (AnalyzerEnums::Edge)value;
        if (text_archive >> value) mClientEnableActiveState[i] = (BitState)value;
    }
    // bool success = text_archive >> mUsePackets;  //new paramater added -- do this for backwards compatibility
    // if( success == false )
    //	mUsePackets = false; //if the archive fails, set the default value

    ClearChannels();
    AddChannel( mMosiChannel, "MOSI", mMosiChannel != UNDEFINED_CHANNEL );
    AddChannel( mMisoChannel, "MISO", mMisoChannel != UNDEFINED_CHANNEL );
    AddChannel( mClockChannel, "CLOCK", mClockChannel != UNDEFINED_CHANNEL );
    AddChannel( mEnableChannel, "ENABLE", mEnableChannel != UNDEFINED_CHANNEL );
    for (U32 i = 0; i < 3; ++i) AddChannel(mExtraCS[i], "CS", mExtraCS[i] != UNDEFINED_CHANNEL);

    UpdateInterfacesFromSettings();
}

const char* SpiAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << "SaleaeSpiMultiCSAnalyzer";
    text_archive << mMosiChannel;
    text_archive << mMisoChannel;
    text_archive << mClockChannel;
    text_archive << mEnableChannel;
    text_archive << mShiftOrder;
    text_archive << mBitsPerTransfer;
    text_archive << mClockInactiveState;
    text_archive << mDataValidEdge;
    text_archive << mEnableActiveState;

    for (U32 i = 0; i < 3; ++i) text_archive << mExtraCS[i];
    for (U32 i = 1; i < kClientCount; ++i) {
        text_archive << mClientShiftOrder[i];
        text_archive << mClientBitsPerTransfer[i];
        text_archive << mClientClockInactiveState[i];
        text_archive << mClientDataValidEdge[i];
        text_archive << mClientEnableActiveState[i];
    }
    return SetReturnString( text_archive.GetString() );
}

void SpiAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mMosiChannelInterface->SetChannel( mMosiChannel );
    mMisoChannelInterface->SetChannel( mMisoChannel );
    mClockChannelInterface->SetChannel( mClockChannel );
    mEnableChannelInterface->SetChannel( mEnableChannel );
    for (U32 i = 0; i < 3; ++i)
        mExtraCSInterfaces[i]->SetChannel(mExtraCS[i]);
    for (U32 i = 0; i < 3; ++i) {
        mClientShiftOrderInterfaces[i]->SetNumber(mClientShiftOrder[i + 1]);
        mClientBitsInterfaces[i]->SetNumber(mClientBitsPerTransfer[i + 1]);
        mClientClockInterfaces[i]->SetNumber(mClientClockInactiveState[i + 1]);
        mClientPhaseInterfaces[i]->SetNumber(mClientDataValidEdge[i + 1]);
        mClientPolarityInterfaces[i]->SetNumber(mClientEnableActiveState[i + 1]);
    }
    mShiftOrderInterface->SetNumber( mShiftOrder );
    mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );
    mClockInactiveStateInterface->SetNumber( mClockInactiveState );
    mDataValidEdgeInterface->SetNumber( mDataValidEdge );
    mEnableActiveStateInterface->SetNumber( mEnableActiveState );
}
