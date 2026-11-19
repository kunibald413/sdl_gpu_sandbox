#!/bin/bash

echo "Compilig shaders..."
./compile_shaders.sh

mkdir -p build

cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build .


