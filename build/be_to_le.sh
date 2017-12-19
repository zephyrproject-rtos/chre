#!/bin/bash
#
# Converts a big-endian hex string to a little-endian hex string.
#
# Example: ./be_to_le.sh 0x12345678
# 0x78563412

BE_VALUE=$1
echo 0x`echo -n ${BE_VALUE:2} | tac -rs ..`
