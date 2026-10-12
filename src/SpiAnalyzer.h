#ifndef SPI_ANALYZER_H
#define SPI_ANALYZER_H

#include <Analyzer.h>
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
   void Setup();
   U32 ReadMuxAddress(U64 sample);
   bool AddressChangedBefore(U64 sample);
   void EmitError(U64 start, U64 end, U32 cs);
   void DecodeTransaction(U64 start, U32 cs);
   bool NextEnableStart(U64 *sample);
   bool EdgeBefore(U64 sample, AnalyzerChannelData *channel);
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
