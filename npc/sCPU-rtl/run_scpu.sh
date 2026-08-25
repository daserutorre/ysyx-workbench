#!/bin/bash
# run_scpu.sh
# Builds and runs the sCPU simulation (npc/sCPU-rtl).
# This script is meant to live inside the sCPU-rtl/ folder itself.
#
# Usage:
#   ./run_scpu.sh          -> builds and runs the sim (make sim)
#   ./run_scpu.sh clean    -> cleans build artifacts (make clean)

set -e  # exit immediately if any command fails

# Resolve the directory this script lives in, so it works regardless of
# where you call it from.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR"

if [ "$1" == "clean" ]; then
  echo "Cleaning sCPU-rtl build artifacts..."
  make clean
else
  echo "Building and running sCPU simulation..."
  make sim
fi