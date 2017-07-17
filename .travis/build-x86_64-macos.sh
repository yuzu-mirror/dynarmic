#!/bin/sh

set -e
set -x
set -o pipefail

export MACOSX_DEPLOYMENT_TARGET=10.9

mkdir build && cd build
cmake .. -GXcode -DDYNARMIC_USE_SYSTEM_BOOST=0 -DBoost_INCLUDE_DIRS=${PWD}/../externals/ext-boost -DDYNARMIC_TESTS=0
xcodebuild -configuration Release
