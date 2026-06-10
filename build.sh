#!/bin/bash
name="" #$(echo $RANDOM | md5sum | head -c 16)
src=$(pwd)
folder="/tmp/os${name}"
mkdir "${folder}"
cd "${folder}"
cmake -G Ninja "${src}"
ninja

echo "build directory at: ${folder}"
