#!/bin/bash

rm -rf build
mkdir -p build
cd build

path="${HOME}/Desktop/GPTool"

cmake -DCMAKE_BUILD_TYPE="Release" -DCMAKE_INSTALL_PREFIX=$path ..

make -j install
