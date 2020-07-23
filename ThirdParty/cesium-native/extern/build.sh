#!/bin/sh

ORIGINALDIR=$PWD
SCRIPT=$(readlink -f "$0")
EXTERNDIR=$(dirname "$SCRIPT")
echo $EXTERNDIR
cd $EXTERNDIR
mkdir -p build
cd build

# Draco
cd $EXTERNDIR
cd build
mkdir -p draco
cd draco
cmake ../../draco/
cmake --build . --config Release

# uriparser
cd $EXTERNDIR
cd build
mkdir -p uriparser
cd uriparser
cmake ../../uriparser/ -DCMAKE_BUILD_TYPE=Release -DURIPARSER_BUILD_TESTS:BOOL=OFF -DURIPARSER_BUILD_DOCS:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DURIPARSER_ENABLE_INSTALL:BOOL=OFF -DURIPARSER_BUILD_TOOLS:BOOL=OFF
cmake --build . --config Release

cd $ORIGINALDIR
