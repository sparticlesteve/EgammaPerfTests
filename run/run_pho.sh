#!/bin/bash

# Run the photon job
date
valgrind --tool=callgrind --cache-sim=yes --instr-atstart=no photonCalibPerf ./xAOD.pool.root 50 2>&1 | tee pho.log
echo "finished"
date
