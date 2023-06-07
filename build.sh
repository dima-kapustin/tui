#!/bin/bash

#git pull
BDIR=build

#export CC=`which gcc-13`
#export CXX=`which g++-13`

cmake -S . -B $BDIR
cmake --build $BDIR --clean-first -- -j4  2>$BDIR.log

