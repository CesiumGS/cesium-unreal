#!/bin/sh

EXTERNDIR=$(dirname "$0")
pushd $EXTERNDIR
mkdir build
cd build
popd

# Draco
pushd $EXTERNDIR; cd build; mkdir draco; cd draco
cmake ../../draco/
cmake --build . --config Release
popd

# uriparser
pushd $EXTERNDIR; cd build; mkdir uriparser; cd uriparser
cmake ../../uriparser/ -DCMAKE_BUILD_TYPE=Release -D URIPARSER_BUILD_TESTS:BOOL=OFF -D URIPARSER_BUILD_DOCS:BOOL=OFF -D BUILD_SHARED_LIBS:BOOL=OFF -D URIPARSER_ENABLE_INSTALL:BOOL=OFF -D URIPARSER_BUILD_TOOLS:BOOL=OFF
cmake --build . --config Release
popd
