#!/bin/bash
set -euo pipefail

source /software/root_install/bin/thisroot.sh
source /usr/local/bin/geant4.sh

cd /opt/G4LArBox
exec ./G4LArBox "$@"
