#ifndef VERTEXGENERATORINTERFACE_HH
#define VERTEXGENERATORINTERFACE_HH

#include "G4ThreeVector.hh"

namespace G4LArBox
{
    class VertexGeneratorInterface
    {
    public:
        virtual ~VertexGeneratorInterface() = default;

        virtual bool HasNextVertex() const;
        virtual G4ThreeVector ShootVertex() = 0;
        virtual void ShootVertex(G4ThreeVector& vertex);
    };
}

#endif // VERTEXGENERATORINTERFACE_HH
