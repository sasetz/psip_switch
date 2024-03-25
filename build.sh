#!/usr/bin/bash

BUILD_DIR="/home/sasetz/projects/cpp/psip_switch/build"

cmake --build $BUILD_DIR --target all
cp "$BUILD_DIR/compile_commands.json" .
