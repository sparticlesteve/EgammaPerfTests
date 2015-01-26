
#include "TFile.h"
#include "TError.h"

// Infrastructure includes
#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODCore/ShallowCopy.h"
#include "PATInterfaces/SystematicSet.h"
#include "PATInterfaces/SystematicsUtil.h"

// EDM includes
#include "xAODEgamma/PhotonContainer.h"
#include "xAODEgamma/ElectronContainer.h"

// CP includes
#include "ElectronPhotonFourMomentumCorrection/EgammaCalibrationAndSmearingTool.h"

// Callgrind control
#include "valgrind/callgrind.h"

// Error checking macro
#define CHECK( ARG )                                 \
  do {                                               \
    const bool result = ARG;                         \
    if(!result) {                                    \
      ::Error(appName, "Failed to execute: \"%s\"", \
              #ARG );                                \
      return 1;                                      \
    }                                                \
  } while( false )

int main(int argc, char* argv[])
{
  // The application's name
  const char* appName = argv[ 0 ];

  // Command line processing
  if(argc < 2) {
    Error(appName, "No file name received!");
    Error(appName, "  usage: %s [xAOD file name]", appName);
    return EXIT_FAILURE;
  }

  // The input xAOD file
  const std::string fileName = argv[1];

  // Initialize the xAOD infrastructure
  CHECK( xAOD::Init(appName) );
  StatusCode::enableFailure();

  // Open the input file
  Info(appName, "Opening file: %s", fileName.c_str());
  std::unique_ptr<TFile> inputFile(TFile::Open(fileName.c_str(), "READ"));
  CHECK( inputFile.get() );

  // Create a TEvent object
  xAOD::TEvent event(xAOD::TEvent::kClassAccess);
  CHECK( event.readFrom(inputFile.get()) );
  Long64_t entries = event.getEntries();
  Info(appName, "Number of events in the file: %lli", entries);

  // Decide how many events to run over
  if(argc > 2) {
    const Long64_t e = atoll(argv[2]);
    entries = std::min(entries, e);
  }
  Info(appName, "Will process %lli events", entries);

  // Configure the tools
  CP::EgammaCalibrationAndSmearingTool
    calibTool("EgammaCalibrationAndSmearingTool");
  CHECK( calibTool.initialize() );

  // TODO: systematics
  CP::SystematicSet recommendedSysts = calibTool.recommendedSystematics();
  std::vector<CP::SystematicSet> sysList =
    CP::make_systematics_vector(recommendedSysts);

  Info(appName, "List of systematics:");
  for(auto& sys : sysList){
    std::string sysName = sys.empty()? "Nominal" : sys.name();
    std::cout << "    " << sysName << std::endl;
  }

  // Event progress printout 
  const int nEvtPrints = 5;
  const int evtPrintFreq = (entries-1) / nEvtPrints + 1;

  // Start Callgrind monitoring
  CALLGRIND_START_INSTRUMENTATION;

  // Loop over events
  for(Long64_t entry = 0; entry < entries; ++entry){

    // Print progress
    if((entry % evtPrintFreq) == 0)
      Info(appName, "===>>> processing event #%lli <<<===", entry);

    // Retrieve the event info
    const xAOD::EventInfo* evtInfo = 0;
    CHECK( event.retrieve(evtInfo, "EventInfo") );

    // Retrieve photon collection
    const xAOD::PhotonContainer* photons = 0;
    CHECK( event.retrieve(photons, "PhotonCollection") );

    // Loop over systematics
    for(auto& sys : sysList){

      // Configure the tool for these sytematics
      CHECK( calibTool.applySystematicVariation(sys) );

      // Create a shallow copy of the photon container
      auto copyPair = xAOD::shallowCopyContainer(*photons);
      xAOD::PhotonContainer* sysPhotons = copyPair.first;

      // Loop over photons
      for(auto photon : *sysPhotons){
        // Apply calibration
        if(calibTool.applyCorrection(*photon, evtInfo) ==
           CP::CorrectionCode::Error){
          Error(appName, "Problem in applyCorrection");
          return EXIT_FAILURE;
        }
      }

    }

  }

  // Stop Callgrind monitoring
  CALLGRIND_STOP_INSTRUMENTATION;

  return EXIT_SUCCESS;
}
