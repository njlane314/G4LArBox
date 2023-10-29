#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH

#include "DetectorConstruction.hh"

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

namespace G4LArBox
{
  class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
  {
    public:
      PrimaryGeneratorAction();
      ~PrimaryGeneratorAction();

      void GeneratePrimaries(G4Event*);

      void BulkVertexGenerator(G4ThreeVector& vtx);
  };
}

#endif // PRIMARYGENERATORACTION_HH