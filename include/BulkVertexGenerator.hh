#ifndef BULKVERTEXGENERATOR_HH
#define BULKVERTEXGENERATOR_HH

#include "VertexGeneratorInterface.hh"

class G4VPhysicalVolume;

namespace G4LArBox
{
    class BulkVertexGenerator final : public VertexGeneratorInterface
    {
    public:
        BulkVertexGenerator() = default;
        explicit BulkVertexGenerator(G4VPhysicalVolume* volume);

        void SetVolume(G4VPhysicalVolume* volume);
        G4ThreeVector ShootVertex() override;

    private:
        G4VPhysicalVolume* volume_ = nullptr;
    };
}

#endif // BULKVERTEXGENERATOR_HH
