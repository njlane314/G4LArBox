#include "PhysicsList.hh"

namespace G4LArBox 
{
    PhysicsList::PhysicsList()
    {
        RegisterPhysics( new G4DecayPhysics() );
        RegisterPhysics( new G4EmStandardPhysics_option4() );
        RegisterPhysics( new G4EmExtraPhysics() );
        RegisterPhysics( new G4HadronElasticPhysics() );
        RegisterPhysics( new G4HadronPhysicsQGSP_BIC() );
        RegisterPhysics( new G4StoppingPhysics() );
        RegisterPhysics( new G4IonElasticPhysics() );
        RegisterPhysics( new G4IonPhysics() );
    }

    PhysicsList::~PhysicsList() 
    {}
}