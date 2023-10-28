#ifndef ACTIONINITIALIZATION_HH
#define ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

namespace G4LArBox
{
  class ActionInitialization : public G4VUserActionInitialization
  {
    public:
      ActionInitialization();
      ~ActionInitialization();

      void BuildForMaster() const override;
      void Build() const override;
  };
}

#endif // ACTIONINITIALIZATION_HH