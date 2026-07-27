#!/bin/bash

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <ROOT filename> [active half-size in mm]"
    exit 1
fi

ROOTFILE=$1
HALF_SIZE_MM=${2:-500}

root -l -b -q "PlotParticleTrajectories.c(\"$ROOTFILE\", $HALF_SIZE_MM)"
root -l -b -q "PlotParticleLifetimes.c(\"$ROOTFILE\")"
