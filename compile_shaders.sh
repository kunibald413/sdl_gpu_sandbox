#!/bin/bash

# reference:
# https://github.com/TheSpydog/SDL_gpu_examples/blob/636f4ff82cb/Content/Shaders/Source/compile.sh

SC=shadercross

SHADERS_DIR=shaders
COMPILED_DIR=$SHADERS_DIR/compiled

mkdir -p "$COMPILED_DIR"

# for vulkan/metal also emit .spv / .msl,
# e.g. "$SC" "$filename" -o "$COMPILED_DIR/${stem/.hlsl/.spv}"
for filename in "$SHADERS_DIR"/*.{vert,frag,comp}.hlsl; do
    [ -f "$filename" ] || continue
    stem=$(basename "$filename")
    "$SC" "$filename" -o "$COMPILED_DIR/${stem/.hlsl/.dxil}"
done