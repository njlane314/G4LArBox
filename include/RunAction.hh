#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "DataHandler.hh"

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

namespace G4LArBox
{
  class RunAction : public G4UserRunAction
  {
    public:
      RunAction();
      ~RunAction();

      void BeginOfRunAction(const G4Run*);
      void EndOfRunAction(const G4Run*);
  };
}

#endif // RUNACTION_HH