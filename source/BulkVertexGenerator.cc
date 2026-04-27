#include "BulkVertexGenerator.hh"

#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4RunManager.hh"
#include "G4VPhysicalVolume.hh"
#include "Randomize.hh"

#include <stdexcept>

namespace G4LArBox
{
    BulkVertexGenerator::BulkVertexGenerator(G4VPhysicalVolume* volume)
        : volume_(volume)
    {}

    void BulkVertexGenerator::SetVolume(G4VPhysicalVolume* volume)
    {
        volume_ = volume;
    }

    G4ThreeVector BulkVertexGenerator::ShootVertex()
    {
        G4VPhysicalVolume* volume = volume_;
        if (volume == nullptr)
        {
            const auto* detector_construction = static_cast<const DetectorConstruction*>(
                G4RunManager::GetRunManager()->GetUserDetectorConstruction());
            if (detector_construction != nullptr)
            {
                volume = detector_construction->GetActiveVolume();
            }
        }

        if (volume == nullptr || volume->GetLogicalVolume() == nullptr)
        {
            throw std::runtime_error("Failed to locate the active detector volume while sampling a generator vertex.");
        }

        auto* box = dynamic_cast<G4Box*>(volume->GetLogicalVolume()->GetSolid());
        if (box == nullptr)
        {
            throw std::runtime_error("Failed to locate a box-shaped active detector volume while sampling a generator vertex.");
        }

        return G4ThreeVector(
            CLHEP::RandFlat::shoot(-box->GetXHalfLength(), box->GetXHalfLength()),
            CLHEP::RandFlat::shoot(-box->GetYHalfLength(), box->GetYHalfLength()),
            CLHEP::RandFlat::shoot(-box->GetZHalfLength(), box->GetZHalfLength()));
    }
}
