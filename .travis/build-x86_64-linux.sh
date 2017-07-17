#!/bin/sh

set -e
set -x

export CC=gcc-6
export CXX=g++-6
export PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH

mkdir build && cd build
cmake .. -DDYNARMIC_USE_SYSTEM_BOOST=0 -DBoost_INCLUDE_DIRS=${PWD}/../externals/ext-boost -DCMAKE_BUILD_TYPE=Release
make -j4

ctest -VV -C Release
