#include "PhysicsList.hh"

#include "G4DecayPhysics.hh"
#include "G4EmExtraPhysics.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronElasticPhysics.hh"
#include "G4HadronPhysicsQGSP_BIC.hh"
#include "G4IonElasticPhysics.hh"
#include "G4IonPhysics.hh"
#include "G4NuclideTable.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4StoppingPhysics.hh"
#include "G4SystemOfUnits.hh"

namespace G4LArBox 
{
    PhysicsList::PhysicsList()
    {
        G4NuclideTable::GetInstance()->SetThresholdOfHalfLife(1.0e10 * year);
        RegisterPhysics(new G4DecayPhysics());
        RegisterPhysics(new G4RadioactiveDecayPhysics());
        RegisterPhysics(new G4EmStandardPhysics_option4());
        RegisterPhysics(new G4EmExtraPhysics());
        RegisterPhysics(new G4HadronElasticPhysics());
        RegisterPhysics(new G4HadronPhysicsQGSP_BIC());
        RegisterPhysics(new G4StoppingPhysics());
        RegisterPhysics(new G4IonElasticPhysics());
        RegisterPhysics(new G4IonPhysics());
    }
}
