#!/bin/sh
name="" #$(echo $RANDOM | md5sum | head -c 16)
src=$(pwd)
folder="/tmp/os${name}"
mkdir "${folder}"
cd "${folder}"
cmake "${src}"
make -j 8
echo "build directory at: ${folder}"
thunar "${folder}"
