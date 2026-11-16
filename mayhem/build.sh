#!/usr/bin/env bash
#
# mayhem/build.sh — build Mandelbulber2's fuzz harnesses and its behavioural oracle. Runs offline
# inside the commit image as `mayhem` in /mayhem; safe to re-run. The core (everything but the
# GUI entry point main.cpp) is built as a static library by mayhem/CMakeLists.txt, which leaves
# upstream's own cmake project untouched.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# 1) the project and the three fuzz harnesses (+ -standalone reproducers), sanitized, DWARF 3.
cmake -S mayhem -B build-fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE= \
  -DCMAKE_C_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link" \
  -DCMAKE_CXX_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link" \
  -DMB_BUILD_FUZZ=ON -DMB_LSAN_OFF="$SRC/mayhem/lsan_off.cc" -DMB_OUT="$SRC" \
  -DMB_FUZZ_ENGINE="$LIB_FUZZING_ENGINE" -DMB_STANDALONE_MAIN="$STANDALONE_FUZZ_MAIN"
cmake --build build-fuzz -j"$MAYHEM_JOBS"

# 2) the oracle and upstream's mandelbulber2 binary (for its QtTest suite), with NORMAL flags (no sanitizers) in a separate tree.
cmake -S mayhem -B build-test -G Ninja \
  -DCMAKE_BUILD_TYPE= \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" \
  -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
  -DMB_BUILD_TEST=ON -DMB_OUT="$SRC/build-test/bin"
cmake --build build-test -j"$MAYHEM_JOBS"
