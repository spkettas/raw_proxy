#!/bin/sh

mode=Debug
#mode=Release

rm -rf build
cmake -DCMAKE_BUILD_TYPE=$mode -B build
cmake --build build -- -j10
