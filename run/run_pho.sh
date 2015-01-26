#!/bin/bash

# Run the photon job
date
valgrind --tool=callgrind --cache-sim=yes --instr-atstart=no photonCalibPerf ./xAOD.pool.root 50
echo "finished"
date
