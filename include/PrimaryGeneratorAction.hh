#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH

#include "GeneratorConfig.hh"
#include "GeneratorTruth.hh"
#include "GenieGSTReader.hh"
#include "CorsikaReader.hh"
#include "DetectorConstruction.hh"
#include "MarleyGenerator.hh"
#include "BxDecay0Generator.hh"
#include "BulkVertexGenerator.hh"

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"
#include "G4GeneralParticleSource.hh"

#include <memory>

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4RunManager.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

namespace G4LArBox
{
  class GeneratorMessenger;

  class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
  {
    public:
        PrimaryGeneratorAction();
        ~PrimaryGeneratorAction() override;

        void GeneratePrimaries(G4Event*) override;
        void BulkVertexGenerator(G4ThreeVector& vtx);

    private:
        void GenerateGeniePrimaries(G4Event* event);
        void GenerateGenieProtonDecayPrimaries(G4Event* event);
        void GenerateCorsikaPrimaries(G4Event* event);
        void GenerateCorsikaGenieOverlayPrimaries(G4Event* event);
        void GenerateMarleyPrimaries(G4Event* event);
        void GenerateBxDecay0Primaries(G4Event* event);
        void GenerateRadiologicalPrimaries(G4Event* event);
        void GenerateRockNeutronPrimaries(G4Event* event);
        int AddRadiologicalBackgrounds(G4Event* event, GeneratorTruthRecord& truth);
        int AddRockNeutronBackgrounds(G4Event* event, GeneratorTruthRecord& truth);
        int AddRadiologicalDecayProducts(G4Event* event,
                                         const RadiologicalIsotopeConfig& isotope,
                                         const G4ThreeVector& vertex,
                                         double decay_time);
        double SampleAllowedBetaKineticEnergy(double endpoint_energy) const;
        double SampleRockNeutronEnergy() const;
        double RockNeutronExpectedCount(double shell_area) const;
        double RockNeutronShellArea() const;
        G4ThreeVector SampleRockNeutronVertex(G4ThreeVector& inward_normal, int& face) const;
        G4ThreeVector SampleRockNeutronDirection(const G4ThreeVector& vertex,
                                                 const G4ThreeVector& inward_normal);
        void AddIsotropicPrimary(G4Event* event,
                                 G4ParticleDefinition* definition,
                                 const G4ThreeVector& vertex,
                                 double time,
                                 double kinetic_energy);
        double ActiveVolumeMassKg() const;
        bool RadiologicalOverlayEnabled() const;
        bool RockNeutronOverlayEnabled() const;
        int AddGenieEvent(G4Event* event,
                          const GenieGSTEvent& genie_event,
                          GeneratorTruthRecord& truth,
                          bool include_primary_lepton);
        int AddCorsikaEvent(G4Event* event, const CorsikaEvent& corsika_event);
        int AddMarleyEvent(G4Event* event,
                           const MarleyEventRecord& marley_event,
                           GeneratorTruthRecord& truth);
        int AddBxDecay0Event(G4Event* event,
                             const BxDecay0EventRecord& bx_event,
                             GeneratorTruthRecord& truth);
        void CaptureTruthFromEvent(const G4Event* event, GeneratorTruthRecord& truth) const;
        G4ParticleDefinition* FindParticleDefinition(int pdg_code) const;
        int DetermineGeniePrimaryLeptonPdg(const GenieGSTEvent& event) const;

        G4GeneralParticleSource* generalgen_;
        GeneratorConfig generator_config_;
        GeneratorMessenger* generator_messenger_;
        std::unique_ptr<GenieGSTReader> genie_reader_;
        std::unique_ptr<CorsikaReader> corsika_reader_;
        std::unique_ptr<MarleyGenerator> marley_generator_;
        std::unique_ptr<BxDecay0Generator> bxdecay0_generator_;
        std::unique_ptr<G4LArBox::BulkVertexGenerator> bulk_vertex_generator_;
  };
}

#endif // PRIMARYGENERATORACTION_HH
