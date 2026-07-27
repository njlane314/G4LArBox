#!/bin/bash
set -euo pipefail

source /opt/root/bin/thisroot.sh
source /opt/geant4/bin/geant4.sh

cd /opt/G4LArBox
exec ./G4LArBox "$@"
