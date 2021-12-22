#!/bin/bash

# Log all commands executed, and stop at the first build failure
set -xe

CFLAGS="-g3" make EXE=libchessutil_debug.a
make clean
CFLAGS="-fsanitize=address -g3" make EXE=libchessutil_asan.a
make clean
CFLAGS="-fsanitize=undefined -g3" make EXE=libchessutil_ubsan.a
make clean
AR=gcc-ar CFLAGS="-flto" make EXE=libchessutil_lto.a