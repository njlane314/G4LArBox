#include "VertexGeneratorInterface.hh"

namespace G4LArBox
{
    bool VertexGeneratorInterface::HasNextVertex() const
    {
        return true;
    }

    void VertexGeneratorInterface::ShootVertex(G4ThreeVector& vertex)
    {
        vertex = ShootVertex();
    }
}
