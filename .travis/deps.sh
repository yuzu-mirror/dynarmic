#!/bin/sh

set -e
set -x

# TODO: This isn't ideal.
cd externals
git clone https://github.com/citra-emu/ext-boost
cd ..

if [ "$TRAVIS_OS_NAME" = "linux" -o -z "$TRAVIS_OS_NAME" ]; then
    mkdir -p $HOME/.local
    curl -L https://cmake.org/files/v3.4/cmake-3.4.1-Linux-i386.tar.gz \
        | tar -xz -C $HOME/.local --strip-components=1
fi
