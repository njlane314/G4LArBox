#include "DataHandler.hh"

namespace G4LArBox 
{
    Hit::Hit(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r)
    {
        tid = step->GetTrack()->GetTrackID();
        pid = step->GetTrack()->GetParentID();
        parid = step->GetTrack()->GetParentID();
        ekin = step->GetPreStepPoint()->GetKineticEnergy();
        edep = step->GetTotalEnergyDeposit();

        G4ThreeVector startPos = step->GetPreStepPoint()->GetPosition();
        xs = startPos.x();
        ys = startPos.y();
        zs = startPos.z();

        G4ThreeVector endPos = step->GetPostStepPoint()->GetPosition();
        xe = endPos.x();
        ye = endPos.y();
        ze = endPos.z();

        len = step->GetStepLength();
        ta = step->GetPreStepPoint()->GetGlobalTime();

        hnexc = nexc;
        hnion = nion;
        hnopt = nopt;
        hntherm = ntherm;
        hr = r;
    }

    int Hit::getTID() const               { return tid; }
    int Hit::getPID() const               { return pid; }
    int Hit::getParID() const             { return parid; }
    double Hit::getEkin() const           { return ekin; }
    double Hit::getEdep() const           { return edep; }
    double Hit::getLength() const         { return len; }
    double Hit::getTime() const           { return ta; }

    int Hit::getNExc() const              { return hnexc; }
    int Hit::getNIon() const              { return hnion; }
    int Hit::getNOpt() const              { return hnopt; }
    int Hit::getNTherm() const            { return hntherm; }
    double Hit::getR() const              { return hr; }

    void Hit::getStartPos(double &x, double &y, double &z) const { x = xs; y = ys; z = zs; }
    void Hit::getEndPos(double &x, double &y, double &z) const   { x = xe; y = ye; z = ze; }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    Event::Event(const G4event* event) 
    {
        x0 = event->GetPrimaryVertex(0)->GetX0();
        y0 = event->GetPrimaryVertex(0)->GetY0();
        z0 = event->GetPrimaryVertex(0)->GetZ0();

        nprim = event->GetNumberOfPrimaryVertex();
    }

    void Event::getInteractionPos(double &x, double &y, double &z) const { x = x0; y = y0; z = z0; }

    void Event::addPrimary(const PrimaryParticle& primary) { primaries.push_back(std::make_unique<PrimaryParticle>(primary)); }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    PrimaryParticle::PrimaryParticle(const G4PrimaryParticle* particle)
    {
        pida = particle->GetPDGcode();
        pdga = particle->GetPDGcode();
        ni = 0;
        ekina = particle->GetKineticEnergy();

        G4ThreeVector startPos = particle->GetPosition();
        xa = startPos.x();
        ya = startPos.y();
        za = startPos.z();

        G4ThreeVector momentum = particle->GetMomentum();
        pxa = momentum.x();
        pya = momentum.y();
        pza = momentum.z();

        ta = particle->GetGlobalTime();
    }

    int PrimaryParticle::getPIDA() const    { return pida; }
    int PrimaryParticle::getPDGA() const    { return pdga; }
    int PrimaryParticle::getNI() const      { return ni; }
    double PrimaryParticle::getTime() const { return ta; }
    double PrimaryParticle::getEkin() const { return ekina; }

    void PrimaryParticle::getStartPosition(double &x, double &y, double &z) const { x = xa; y = ya; z = za; }
    void PrimaryParticle::getMomentum(double &px, double &py, double &pz) const { px = pxa; py = pya; pz = pza; }
    void PrimaryParticle::addSecondary(const SecondaryParticle& secondary)    { secondaries.push_back(std::make_unique<SecondaryParticle>(secondary)); }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    SecondaryParticle::SecondaryParticle(const G4Track* track)
    {
        pidi = track->GetDefinition()->GetPDGEncoding();
        pdgi = track->GetDefinition()->GetPDGEncoding();
        parid = track->GetParentID();
        ekini = track->GetKineticEnergy();

        G4ThreeVector startPos = track->GetPosition();
        xi = startPos.x();
        yi = startPos.y();
        zi = startPos.z();

        G4ThreeVector momentum = track->GetMomentum();
        pxi = momentum.x();
        pyi = momentum.y();
        pzi = momentum.z();

        ti = track->GetGlobalTime();
    }

    int SecondaryParticle::getPID() const      { return pidi; }
    int SecondaryParticle::getPDG() const      { return pdgi; }
    int SecondaryParticle::getParentID() const { return parid; }
    double SecondaryParticle::getTime() const  { return ti; }
    double SecondaryParticle::getEkin() const  { return ekini; }

    void SecondaryParticle::getStartPosition(double &x, double &y, double &z) const { x = xi; y = yi; z = zi; }
    void SecondaryParticle::getMomentum(double &px, double &py, double &pz) const   { px = pxi; py = pyi; pz = pzi; }
    void SecondaryParticle::addHit(const Hit& hit)                                  { hits.push_back(std::make_unique<Hit>(hit)); }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    DataHandler* DataHandler::instance = nullptr;

    DataHandler* DataHandler::GetInstance() 
    {
        if (!instance) 
        {
            instance = new DataHandler("output.root");
        }
        return instance;
    }

    DataHandler::DataHandler(const char* filename)
        : rootFile(nullptr), hitTree(nullptr), primaryTree(nullptr), secondaryTree(nullptr) 
    {
        rootFile = new TFile(filename, "RECREATE");

        hitTree = new TTree("hitTree", "Tree of hits");
        hitTree->Branch("hit", &hit, "tid/I:pid/I:parid/I:ekin/D:edep/D:xs/D:ys/D:zs/D:xe/D:ye/D:ze/D:len/D:ta/D");

        primaryTree = new TTree("primaryTree", "Tree of primaries");
        primaryTree->Branch("primary", &primary, "pida/I:pdga/I:ni/I:xa/D:ya/D:za/D:ta/D:pxa/D:pya/D:pza/D:ekina/D");

        secondaryTree = new TTree("secondaryTree", "Tree of secondaries");
        secondaryTree->Branch("secondary", &secondary, "pidi/I:pdgi/I:parid/I:xi/D:yi/D:zi/D:ti/D:pxi/D:pyi/D:pzi/D:ekini/D");
    }

    DataHandler::~DataHandler() 
    {
        if (rootFile) 
        {
            rootFile->Close();
            delete rootFile;
        }
    }

    void DataHandler::AddHit(const G4Step* step, int nexc, int nion, double r) 
    {
        hits.push_back(Hit(step, nexc, nion, r));
    }

    void DataHandler::AddPrimary(const PrimaryParticle& primary) 
    {
        primaries.push_back(PrimaryParticle(primary));
    }

    void DataHandler::AddSecondary(const G4Track* track) 
    {
        secondaries.push_back(SecondaryParticle(track));
    }

    void DataHandler::WriteToFile() 
    {
        for (const auto& hit : hits) 
        {
            this->hit = hit;
            hitTree->Fill();
        }

        for (const auto& primary : primaries) 
        {
            this->primary = primary;
            primaryTree->Fill();
        }

        for (const auto& secondary : secondaries) 
        {
            this->secondary = secondary;
            secondaryTree->Fill();
        }

        rootFile->Write();
    }

    void DataHandler::Reset() 
    {
        hits.clear();
        primaries.clear();
        secondaries.clear();
    }
}