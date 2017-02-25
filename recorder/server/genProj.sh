#!/bin/bash   
gyp --depth=. --generator-output=build  src/main.gyp
if [ -d build ]; then
cd build
make
fi

