#!/bin/bash

# Check if the user provided an argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <ROOT filename>"
    exit 1
fi

# Extract the ROOT filename from the command-line argument
ROOTFILE=$1

# Run the plotting scripts
root -l -b -q "PlotParticleTrajectories.c(\"$ROOTFILE\")"
root -l -b -q "PlotParticleLifetimes.c(\"$ROOTFILE\")"