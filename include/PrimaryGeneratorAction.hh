#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4Box;

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