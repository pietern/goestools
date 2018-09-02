#!/bin/bash

set -ex

git submodule update --init

mkdir -p build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr/local
make -j2
make install
