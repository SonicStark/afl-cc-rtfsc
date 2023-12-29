#!/bin/bash

set -euxo pipefail

data=$1
repo=$2.patched

cp -r $2 $repo

rm -f $repo/src/afl-cc.c
rm -f $repo/GNUmakefile.llvm

cp $data/GNUmakefile.llvm $repo

cp $data/afl-cc.h $repo/include
cp $data/afl-cc.c $repo/src
cp $data/afl-cc-*.c $repo/src
