#!/usr/bin/env bash
# mayhem/build.sh — build cpu_features' fuzz harnesses and upstream's test suite.
#
# cpu_features is a tiny C99 library; the fuzz build compiles its sources
# directly (with $SANITIZER_FLAGS $DEBUG_FLAGS, so the library is instrumented)
# using upstream's own test seams: CPU_FEATURES_MOCK_FILESYSTEM (files),
# CPU_FEATURES_TEST (hwcaps / platform strings) and CPU_FEATURES_MOCK_CPUID_X86.
# Non-x86 implementations are compiled on the x86_64 host with
# -include mayhem/harnesses/arch_shim.h -D<arch macro>.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
cd "$SRC"

OUT="$SRC/build/fuzz"          # build/ is gitignored upstream
mkdir -p "$OUT"
CFLAGS_F="$SANITIZER_FLAGS $DEBUG_FLAGS -O1 -std=gnu99 -Iinclude -Iinclude/internal -Indk_compat \
  -DSTACK_LINE_READER_BUFFER_SIZE=1024 -DCPU_FEATURES_TEST -DCPU_FEATURES_MOCK_FILESYSTEM"
COMMON_SRC="src/stack_line_reader.c src/string_view.c src/hwcaps.c"

"$CC" $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.c -o "$OUT/lsan_off.o"
"$CC" $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"

# build_target <name> "<extra cflags>" <sources...>
# Objects are compiled twice: with SanCov (-fsanitize=fuzzer-no-link) for the
# libFuzzer binary, and without it for the -standalone reproducer, so the
# reproducer also links when SANITIZER_FLAGS is empty.
build_target() {
  local name="$1" extra="$2"; shift 2
  local fobjs=() sobjs=() s o
  mkdir -p "$OUT/$name"
  for s in "$@"; do
    o="$OUT/$name/$(basename "${s%.c}")"
    "$CC" $CFLAGS_F -fsanitize=fuzzer-no-link $extra -c "$s" -o "$o.fuzz.o"
    "$CC" $CFLAGS_F $extra -c "$s" -o "$o.o"
    fobjs+=("$o.fuzz.o"); sobjs+=("$o.o")
  done
  "$CC" $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "${fobjs[@]}" "$OUT/lsan_off.o" \
    -lpthread -o "$SRC/$name"
  "$CC" $SANITIZER_FLAGS $DEBUG_FLAGS "${sobjs[@]}" "$OUT/standalone_main.o" "$OUT/lsan_off.o" \
    -lpthread -o "$SRC/$name-standalone"
}

SHIM="-include mayhem/harnesses/arch_shim.h"
build_target fuzz_x86 "-DCPU_FEATURES_MOCK_CPUID_X86" \
  mayhem/harnesses/fuzz_x86.c src/impl_x86_linux_or_android.c $COMMON_SRC
build_target fuzz_arm "$SHIM -D__arm__" \
  mayhem/harnesses/fuzz_arm.c src/impl_arm_linux_or_android.c $COMMON_SRC
build_target fuzz_ndk_arm "$SHIM -D__arm__" \
  mayhem/harnesses/fuzz_ndk_arm.c src/impl_arm_linux_or_android.c $COMMON_SRC
build_target fuzz_aarch64 "$SHIM -D__aarch64__" \
  mayhem/harnesses/fuzz_aarch64.c src/impl_aarch64_linux_or_android.c $COMMON_SRC
build_target fuzz_mips "$SHIM -D__mips__" \
  mayhem/harnesses/fuzz_mips.c src/impl_mips_linux_or_android.c $COMMON_SRC
build_target fuzz_ppc "$SHIM -D__powerpc__" \
  mayhem/harnesses/fuzz_ppc.c src/impl_ppc_linux.c $COMMON_SRC
build_target fuzz_riscv "$SHIM -D__riscv" \
  mayhem/harnesses/fuzz_riscv.c src/impl_riscv_linux.c $COMMON_SRC
build_target fuzz_s390x "$SHIM -D__s390x__" \
  mayhem/harnesses/fuzz_s390x.c src/impl_s390x_linux.c $COMMON_SRC
build_target fuzz_loongarch "$SHIM -D__loongarch64" \
  mayhem/harnesses/fuzz_loongarch.c src/impl_loongarch_linux.c $COMMON_SRC

# Upstream's gtest suite with normal flags (googletest from the Debian package,
# no download): string_view, stack_line_reader, bit_utils and cpuinfo_x86 tests.
cmake -S mayhem/tests -B build/tests -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build build/tests -j"$MAYHEM_JOBS"
