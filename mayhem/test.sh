#!/usr/bin/env bash
# mayhem/test.sh — run upstream's own gtest suite (built by mayhem/build.sh)
# and report CTRF counts. The suites assert parsed values (feature bits,
# vendor/brand strings, cache descriptors, line splitting), so a neutered
# library fails them.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

T="$SRC/build/tests/cpu_features/test"
passed=0; failed=0; skipped=0
for t in bit_utils_test string_view_test stack_line_reader_test cpuinfo_x86_test; do
  if [ ! -x "$T/$t" ]; then
    echo "missing test binary $T/$t (mayhem/build.sh should have built it)" >&2
    failed=$((failed + 1)); continue
  fi
  out="$("$T/$t" 2>&1)"; rc=$?
  printf '%s\n' "$out"
  # gtest prints "[  PASSED  ] N tests." / "[  FAILED  ] N tests, listed below:"
  p=$(printf '%s\n' "$out" | sed -n 's/^\[  PASSED  \] \([0-9]*\) test.*/\1/p' | tail -1)
  f=$(printf '%s\n' "$out" | sed -n 's/^\[  FAILED  \] \([0-9]*\) test.*/\1/p' | tail -1)
  s=$(printf '%s\n' "$out" | sed -n 's/^\[  SKIPPED \] \([0-9]*\) test.*/\1/p' | tail -1)
  if [ -z "$p" ] && [ -z "$f" ]; then
    echo "$t produced no gtest summary (rc=$rc)" >&2
    failed=$((failed + 1)); continue
  fi
  passed=$((passed + ${p:-0})); failed=$((failed + ${f:-0})); skipped=$((skipped + ${s:-0}))
  if [ "$rc" -ne 0 ] && [ -z "$f" ]; then failed=$((failed + 1)); fi
done
emit_ctrf "gtest" "$passed" "$failed" "$skipped"
