#!/bin/bash
cd link_emulator/
make clean
make
cd ..
make clean
make
./run_experiment.sh 
