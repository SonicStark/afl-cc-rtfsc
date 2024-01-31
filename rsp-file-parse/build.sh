#!/bin/bash

set -eu

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

COMMON_FLAGS="-Wno-pointer-sign -DDETECT_LEAKS=0 -I$SCRIPT_DIR/../release-4.08c-src/include"
DEBUG_FLAGS="-gdwarf-4 -O0 -fsanitize=address"

$CC_CLANG $COMMON_FLAGS $DEBUG_FLAGS \
            -o $SCRIPT_DIR/build/afl-rsp $SCRIPT_DIR/afl.c

$CC_CLANG $COMMON_FLAGS $DEBUG_FLAGS \
            -o $SCRIPT_DIR/build/gcc-rsp $SCRIPT_DIR/gcc.c \
            -Wl,-liberty 

$CXX_CLANG `$LLVM_CONFIG --cxxflags` $COMMON_FLAGS $DEBUG_FLAGS \
            -Wl,-L`$LLVM_CONFIG --libdir` -Wl,-l:libLLVM.so -Wl,-l:libclang-cpp.so \
            -o $SCRIPT_DIR/build/clg-rsp $SCRIPT_DIR/clang.cc

$CC_AFL_LTO $COMMON_FLAGS -fsanitize=address \
            -o $SCRIPT_DIR/build/afl-rsp-fuzz $SCRIPT_DIR/afl.c
