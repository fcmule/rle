#!/bin/sh

set -e
mkdir -p build

clang src/rle.c -Wall -Wpedantic -O3 -march=native -o build/rle
