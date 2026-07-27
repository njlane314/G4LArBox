#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"

#include <memory>

class G4VPhysicalVolume;

namespace G4LArBox
{
    class Messenger;

    class DetectorConstruction final : public G4VUserDetectorConstruction
    {
    public:
        DetectorConstruction();
        ~DetectorConstruction() override;

        G4VPhysicalVolume* Construct() override;
        G4VPhysicalVolume* GetWorldVolume() const { return world_volume_; }
        G4VPhysicalVolume* GetActiveVolume() const { return active_volume_; }

    private:
        std::unique_ptr<Messenger> messenger_;
        G4VPhysicalVolume* world_volume_ = nullptr;
        G4VPhysicalVolume* active_volume_ = nullptr;
    };
}

#endif
