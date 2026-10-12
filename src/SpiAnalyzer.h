#ifndef SPI_ANALYZER_H
#define SPI_ANALYZER_H

#include <Analyzer.h>
#include <memory>
#include "SpiAnalyzerResults.h"
#include "SpiSimulationDataGenerator.h"

class SpiAnalyzerSettings;

class SpiAnalyzer : public Analyzer2
{
 public:
   SpiAnalyzer();
   virtual ~SpiAnalyzer();
   virtual void SetupResults();
   virtual void WorkerThread();
   virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate,
                                       SimulationChannelDescriptor **simulation_channels);
   virtual U32 GetMinimumSampleRateHz();
   virtual const char *GetAnalyzerName() const;
   virtual bool NeedsRerun();

 private:
   void SetupChannels();
   U32 ReadAddress(U64 sample);
   void DecodeSelection(U64 start, U64 end, U32 cs);
   void EmitWord(U32 cs, U64 start, U64 end, U64 mosi, U64 miso);

   std::auto_ptr<SpiAnalyzerSettings> mSettings;
   std::auto_ptr<SpiAnalyzerResults> mResults;
   SpiSimulationDataGenerator mSimulationDataGenerator;
   bool mSimulationInitialized;
   AnalyzerChannelData *mClock;
   AnalyzerChannelData *mMosi;
   AnalyzerChannelData *mMiso;
   AnalyzerChannelData *mEnable;
   AnalyzerChannelData *mAddress[4];
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif
