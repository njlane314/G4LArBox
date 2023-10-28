#ifndef DATAHANDLER_HH
#define DATAHANDLER_HH

#include <vector>
#include <memory>
#include <string>

namespace G4LArBox 
{
    class Hit 
    {
    private:
        int tid; // Track identifier
        int pid; // Particle identifier
        int parid; // Parent particle identifier
        double ekin; // Kinetic energy
        double edep; // Energy deposited
        double xs, ys, zs; // Start positions
        double xe, ye, ze; // End positions
        double len; // Step length
        double ta; // Initial time

        int hnexc; // Hit number of excitons
        int hnion; // Hit number of ions
        int hnopt; // Hit number of optical photons
        int hntherm; // Hit number of thermal electrons
        double hr; // Hit charge recombination

    public:
        Hit(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r);

        int getTID() const;
        int getPID() const;
        int getParID() const;
        double getEkin() const;
        double getEdep() const;
        void getStartPos(double &x, double &y, double &z) const;
        void getEndPos(double &x, double &y, double &z) const;
        double getLength() const;
        double getTime() const;

        int getNExc() const;
        int getNIon() const;
        int getNOpt() const;
        int getNTherm() const;
        double getR() const;
    };

    class Event 
    {
    private: 
        int totnexc; // Total number of excitons
        int totnion; // Total number of ions
        int totnopt; // Total number of optical photons
        int totntherm; // Total number of thermal electrons
        double avgr; // Average charge recombination

        double nprim; // Number of primary particles

        double x0, y0, z0; // Interaction position

        std::vector<std::unique_ptr<PrimaryParticle>> primaries; // Primary particles
    }

    class PrimaryParticle 
    {
    private:
        int pida; // Primary particle identifier
        int pdga; // Primary particle type
        int ni; // Number of secondary particles
        double xa, ya, za; // Primary particle start position
        double ta; // Primary particle initial time
        double pxa, pya, pza; // Primary particle initial momentum
        double ekina; // Primary particle initial kinetic energy

        std::vector<std::unique_ptr<SecondaryParticle>> secondaries; // Secondary particles

    public:
        PrimaryParticle(const G4PrimaryParticle* particle);

        int getPIDA() const;
        int getPDGA() const;
        int getNI() const;
        void getStartPosition(double &x, double &y, double &z) const;
        double getTime() const;
        void getMomentum(double &px, double &py, double &pz) const;
        double getEkin() const;

        void addSecondary(); // increase the number of secondary particles
    };

    class SecondaryParticle 
    {
    private:
        int pidi; // Particle identifier
        int pdgi; // Particle type
        int parid; // Parent particle identifier
        int nh; // Number of hits
        double xi, yi, zi; // Particle start position
        double ti; // Particle initial time
        double pxi, pyi, pzi; // Particle initial momentum
        double ekini; // Particle initial kinetic energy

        std::vector<std::unique_ptr<Hit>> hits; // Hits

    public:
        SecondaryParticle(const G4Track* track);

        int getPID() const;
        int getPDG() const;
        int getParentID() const;
        void getStartPosition(double &x, double &y, double &z) const;
        double getTime() const;
        void getMomentum(double &px, double &py, double &pz) const;
        double getEkin() const;

        void addHit(); // increase the number of hits
    };
} 

#endif  // DATAHANDLER_HH