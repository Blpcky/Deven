#!/bin/sh
# Builds the C++ MVC app to WebAssembly and drops the output
# (skinapp.js + skinapp.wasm) next to index.html so GitHub Pages
# can serve it as plain static files.
#
# Requires emsdk to be installed and activated:
#   git clone https://github.com/emscripten-core/emsdk.git
#   cd emsdk && ./emsdk install latest && ./emsdk activate latest
#   source ./emsdk_env.sh
set -e
cd "$(dirname "$0")"

em++ src/main.cpp \
  -O2 \
  -std=c++17 \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
  -sALLOW_MEMORY_GROWTH=1 \
  -sENVIRONMENT=web \
  -sMODULARIZE=0 \
  -o ../skinapp.js

echo "Built ../skinapp.js and ../skinapp.wasm"
