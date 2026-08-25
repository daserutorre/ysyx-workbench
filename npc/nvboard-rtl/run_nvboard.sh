#!/bin/bash
# run_nvboard.sh
# Launches the nvboard-rtl simulation (two-way switch / running lights demo).
#
# Usage:
#   ./run_nvboard.sh          -> builds and runs the sim (make sim)
#   ./run_nvboard.sh clean    -> cleans build artifacts (make clean)

set -e  # exit immediately if any command fails

NVBOARD_RTL_DIR="$HOME/Documents/ysyx-workbench/npc/nvboard-rtl"

if [ ! -d "$NVBOARD_RTL_DIR" ]; then
  echo "Error: directory not found: $NVBOARD_RTL_DIR"
  exit 1
fi

cd "$NVBOARD_RTL_DIR"

if [ "$1" == "clean" ]; then
  echo "Cleaning nvboard-rtl build artifacts..."
  make clean
else
  echo "Building and running nvboard-rtl simulation..."
  make sim
fi