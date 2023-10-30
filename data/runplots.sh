#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <ROOT filename>"
    exit 1
fi

ROOTFILE=$1

root -l -b -q "PlotParticleTrajectories.c(\"$ROOTFILE\")"
root -l -b -q "PlotParticleLifetimes.c(\"$ROOTFILE\")"