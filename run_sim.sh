#!/bin/bash

# Quit if any command produces an error.
set -e

# Build and run the CHRE simulator.
JOB_COUNT=$((`grep -c ^processor /proc/cpuinfo`))

make google_x86_linux_debug -j$JOB_COUNT
./out/google_x86_linux_debug/libchre
