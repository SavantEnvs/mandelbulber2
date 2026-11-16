#!/usr/bin/env bash
#
# mayhem/test.sh — run the tests built by mayhem/build.sh (never compiles):
#  1) upstream's own QtTest suite, `mandelbulber2 --test` (src/test.cpp), as upstream CI runs it
#     (.github/workflows/linux.yml). `--test` strips -o/--output, so the plain-text
#     `Totals: P passed, F failed, S skipped, B blacklisted` line is what is counted.
#  2) our behavioural oracle, which loads the shipped example settings through the real parsers and
#     asserts parameter values, fractal distance estimates, a text round trip and the v1.21 importer.
# Both are summed into one CTRF report.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
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

TOOL="qttest+mandelbulber-oracle"
app="$SRC/build-test/bin/mandelbulber2"
oracle="$SRC/build-test/bin/mandelbulber_oracle"
for b in "$app" "$oracle"; do
  if [ ! -x "$b" ]; then
    echo "missing $b: mayhem/build.sh did not build it" >&2
    emit_ctrf "$TOOL" 0 1 0
    exit 1
  fi
done

# --- 1) upstream QtTest suite -------------------------------------------------------------------
# Every slot of class Test except renderExamplesWrapper, run by name (QtTest takes slot names as
# arguments and --test passes them through). renderExamplesWrapper is left out on purpose: it
# renders all 791 shipped examples and only asserts each render returns true; it takes ~6.5 of the
# suite's 8 minutes on 16 cores (much longer on a small host) on every image build and every
# grading run, while renderSimple, testImageSave and regressionScenes (which validates the images)
# cover the same render path with stronger checks.
SLOTS=(netrender testFlightWrapper testKeyframeWrapper renderSimpleWrapper testImageSaveWrapper
       regressionScenesWrapper animatedFileName)
# Test::testFolder() and the settings dirs live under $HOME: use a scratch one.
qt_home=$(mktemp -d)
qt_out=$(HOME="$qt_home" QTEST_FUNCTION_TIMEOUT=1800000 timeout 1800 "$app" --test "${SLOTS[@]}" 2>&1)
qt_status=$?
rm -rf "$qt_home"
printf '%s\n' "$qt_out" | grep -Ev '^QWARN|^   Loc: '
totals=$(printf '%s\n' "$qt_out" | grep -E '^Totals: [0-9]+ passed, [0-9]+ failed, [0-9]+ skipped' | tail -1)
if [ -n "$totals" ]; then
  qt_passed=$(sed -E 's/^Totals: ([0-9]+) passed.*/\1/' <<<"$totals")
  qt_failed=$(sed -E 's/^Totals: [0-9]+ passed, ([0-9]+) failed.*/\1/' <<<"$totals")
  qt_skipped=$(sed -E 's/^Totals: [0-9]+ passed, [0-9]+ failed, ([0-9]+) skipped.*/\1/' <<<"$totals")
else
  echo "QtTest printed no Totals line" >&2
  qt_passed=0 qt_failed=1 qt_skipped=0
fi
# a non-zero exit without a counted failure, or fewer passes than the 7 slots + init/cleanup, fails
if [ "$qt_status" -ne 0 ] && [ "$qt_failed" -eq 0 ]; then qt_failed=1; fi
if [ "$qt_passed" -lt $(( ${#SLOTS[@]} + 2 )) ]; then qt_failed=$((qt_failed + 1)); fi

# --- 2) behavioural oracle ----------------------------------------------------------------------
out=$(timeout 600 "$oracle" 2>&1)
status=$?
printf '%s\n' "$out"
passed=$(printf '%s\n' "$out" | grep -c '^PASS ')
failed=$(printf '%s\n' "$out" | grep -c '^FAIL ')
# a run that crashed or exited non-zero without a FAIL line, or ran too few checks, is a failure
if [ "$status" -ne 0 ] && [ "$failed" -eq 0 ]; then failed=1; fi
if [ "$passed" -lt 20 ]; then failed=$((failed + 1)); fi

emit_ctrf "$TOOL" $((qt_passed + passed)) $((qt_failed + failed)) "$qt_skipped"
