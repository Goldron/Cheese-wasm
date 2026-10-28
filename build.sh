#!/usr/bin/env bash
# Compile le moteur d'échecs Cheese (dossier cheese/, sources d'origine) en WebAssembly avec Emscripten :
# dist/cheese.js et dist/cheese.wasm, tels qu'utilisés par CocoChess.
#
#   ./build.sh            (Emscripten dans ~/emsdk, ou EMSDK=<dossier>)
#
# Mêmes options que celles de l'auteur pour Emscripten (cheese/app/CMakeLists.txt), avec trois
# changements :
# - sans POPCNT ni tables de finales (instructions x86, fichiers absents d'une page web) ;
# - préambule corrigé (wasm/preamble.js : point-virgule de tête, nécessaire avec Emscripten 4 et plus) ;
# - sortie dans dist/ sous un nom fixe.
# Le moteur utilise des threads (SharedArrayBuffer) : la page qui le charge doit être « cross-origin
# isolated » (en-têtes COOP/COEP).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")" && pwd)
SRC="$ROOT/cheese"
BUILD="$ROOT/build"
OUT="$ROOT/dist"
# shellcheck disable=SC1091
source "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" > /dev/null 2>&1
command -v em++ > /dev/null || { echo "Emscripten introuvable (EMSDK=<dossier de emsdk>)" >&2; exit 1; }
em++ --version | head -1

# CMake réécrit cheese/src/makeconfig.h selon les options : la version d'origine est remise en place
# à la fin, pour que cheese/ reste identique aux sources de l'auteur
cp "$SRC/src/makeconfig.h" "$ROOT/.makeconfig.h.orig"
trap 'mv -f "$ROOT/.makeconfig.h.orig" "$SRC/src/makeconfig.h"' EXIT

mkdir -p "$BUILD" "$OUT"
cd "$BUILD"
emcmake cmake "$SRC" -DCMAKE_BUILD_TYPE=Release -DUSE_TABLEBASES=OFF -DENABLE_IPO=OFF -DUSE_POPCOUNT=OFF \
  -DBUILD_DYNAMIC=OFF > cmake.log
emmake make -j4 cheese_lib > make.log
emmake make -C app main.cpp.o >> make.log
em++ -O3 -DNDEBUG -sASSERTIONS=0 -sSTACK_SIZE=65536 -sPTHREAD_POOL_SIZE=1 -pthread -sPROXY_TO_PTHREAD=1 \
  -sINITIAL_MEMORY=64MB -sALLOW_MEMORY_GROWTH=0 -sMODULARIZE=1 -sDISABLE_EXCEPTION_CATCHING=1 -sEXPORT_ES6=1 \
  -sALLOW_BLOCKING_ON_MAIN_THREAD=0 -sSTRICT -sWASM=1 -sENVIRONMENT=web,worker -sEXPORT_NAME=Cheese --closure 1 \
  -sEXIT_RUNTIME=1 -sFILESYSTEM=0 -sINCOMING_MODULE_JS_API="[print, printErr]" \
  -s"EXPORTED_RUNTIME_METHODS=['ccall']" --pre-js "$ROOT/wasm/preamble.js" \
  -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive \
  "$BUILD/app/CMakeFiles/cheese_app.dir/main.cpp.o" "$BUILD/src/libcheese_lib.a" -o "$OUT/cheese.js"
cd "$OUT"
sha256sum cheese.js cheese.wasm
