#!/bin/sh
mkdir SourceSim
cp basic-app/pdex.so SourceSim/pdex.so
pdc -sdkpath $2 SourceSim basic-app-sim.pdx
rm -rf SourceSim
