#!/bin/bash

mkdir -p build
cd build

path="${HOME}/Desktop/GPTool"

cmake -DCMAKE_INSTALL_PREFIX=$path ..

make -j install
