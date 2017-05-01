#!/bin/bash

# Quit if any command produces an error.
set -e

# Build and run the CHRE unit test binary.
JOB_COUNT=$((`grep -c ^processor /proc/cpuinfo`))

# TODO: Support header dependencies in the build. Until then, do a clean build
# before each invocation.

make clean
make google_x86_googletest_debug -j$JOB_COUNT
./out/google_x86_googletest_debug/libchre $1
