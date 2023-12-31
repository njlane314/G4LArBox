#ifndef ACTION_INITIALISATION_HH
#define ACTION_INITIALISATION_HH

#include "G4VUserActionInitialization.hh"

namespace G4LArBox {
	class ActionInitialisation : public G4VUserActionInitialization {
	public:
		ActionInitialisation();
		virtual ~ActionInitialisation();

		virtual void Build() const;
	};
}

#endif // ACTION_INITIALISATION_HH