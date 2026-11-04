#!/usr/bin/env bash
#
# Run each unit test suite in its own process, so that a crash or a hang only
# costs that suite, and compare the result with known_test_failures.txt.
#
# The job fails if a suite that is not listed fails. Listed suites may fail
# without failing the job, and are reported when they pass so that they can be
# removed from the list.
#
# usage: run_unit_tests.sh <test binary>

export LC_ALL=C
set -u

BIN=$1
HERE=$(cd "$(dirname "$0")" && pwd)
KNOWN="$HERE/known_test_failures.txt"
LOGS="$PWD/ci-logs/unit"
SUITE_TIMEOUT=${SUITE_TIMEOUT:-300}
JOBS=${JOBS:-4}

if command -v timeout >/dev/null; then
  TIMEOUT=timeout
elif command -v gtimeout >/dev/null; then
  TIMEOUT=gtimeout
else
  echo "::error::no timeout command available"
  exit 2
fi

mkdir -p "$LOGS"
RESULTS="$LOGS/results.txt"
: > "$RESULTS"

"$BIN" --list_content 2>&1 | tr -d '\r' | grep -E '^[a-zA-Z_0-9]+\*?$' | tr -d '*' > "$LOGS/suites.txt"
TOTAL=$(wc -l < "$LOGS/suites.txt")
if [ "$TOTAL" -eq 0 ]; then
  echo "::error::no test suites found in $BIN"
  exit 2
fi

export BIN LOGS RESULTS SUITE_TIMEOUT TIMEOUT
# shellcheck disable=SC2016
xargs -P "$JOBS" -I{} bash -c '
  "$TIMEOUT" "$SUITE_TIMEOUT" "$BIN" --run_test={} --color_output=no > "$LOGS/{}.txt" 2>&1
  echo "{} $?" >> "$RESULTS"
' < "$LOGS/suites.txt"

listed() {
  sed -e 's/#.*//' "$KNOWN" | tr -d ' \r' | grep -qx "$1"
}

new=0 known=0 passed=0
while read -r suite code; do
  if [ "$code" -eq 0 ]; then
    passed=$((passed + 1))
    if listed "$suite"; then
      echo "::notice::$suite passes but is listed in known_test_failures.txt"
    fi
  elif listed "$suite"; then
    known=$((known + 1))
    echo "known failure: $suite (exit $code)"
  else
    new=$((new + 1))
    echo "::error::$suite failed (exit $code)"
    tail -n 40 "$LOGS/$suite.txt"
  fi
done < <(sort "$RESULTS")

echo "$TOTAL suites: $passed passed, $known known failures, $new new failures"
[ "$new" -eq 0 ]
