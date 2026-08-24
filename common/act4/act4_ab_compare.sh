#!/bin/bash
# =====================================================================
# act4_ab_compare.sh — run suites under both trap handlers and diff
#
# Builds and runs each named suite twice, once on the suite's own trap
# handler and once on the ACT4 one, and compares the per-test verdicts.
# Identical output means the ACT4 handler is a drop-in replacement for
# that suite; anything else is a real behavioural difference.
#
# Usage:
#   common/act4/act4_ab_compare.sh [-c CONFIG] [-s SIM] SUITE...
#
#   -c CONFIG   platform config      (default: spike-rv64-max)
#   -s SIM      simulator make verb  (default: spike)
# =====================================================================
set -u

CONFIG=spike-rv64-max
SIM=spike
while getopts "c:s:" opt; do
  case $opt in
    c) CONFIG=$OPTARG ;;
    s) SIM=$OPTARG ;;
    *) echo "usage: $0 [-c CONFIG] [-s SIM] SUITE..." >&2; exit 2 ;;
  esac
done
shift $((OPTIND - 1))
[ $# -gt 0 ] || { echo "usage: $0 [-c CONFIG] [-s SIM] SUITE..." >&2; exit 2; }

ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# Serialise against another copy of this script. Each run does a full
# `make clean` in $ROOT before every build, so two concurrent runs
# delete each other's objects and can link a mixed ELF -- which shows up
# as a divergence that is not real. Fail fast rather than produce
# results nobody can trust.
LOCK="$ROOT/.act4_ab_compare.lock"
exec 9>"$LOCK"
if ! flock -n 9; then
    echo "error: another act4_ab_compare.sh is running in $ROOT" >&2
    echo "       (it rebuilds the whole tree; wait for it or kill it)" >&2
    exit 3
fi

OUT=$(mktemp -d)
trap 'rm -rf "$OUT"; rm -f "$LOCK"' EXIT

# Keep only the per-test verdicts and the summary counters. Test names
# are stable across handlers; timing and addresses are not.
filter() { grep -E '^\[(PASS|FAIL|SKIP)\]|^  (Total|Passed|Failed|Skipped|Assertions):'; }

run_one() {   # $1=suite $2=handler
    ( cd "$ROOT" && make -s clean >/dev/null 2>&1
      timeout 900 make "$SIM-$1" CONFIG="$CONFIG" TRAP_HANDLER="$2" 2>&1 )
}

rc=0
printf '%-28s %-10s %-10s %s\n' SUITE DAMO ACT4 VERDICT
printf '%.0s-' {1..72}; echo

for suite in "$@"; do
    run_one "$suite" damo > "$OUT/$suite.damo.raw" 2>&1
    run_one "$suite" act4 > "$OUT/$suite.act4.raw" 2>&1
    filter < "$OUT/$suite.damo.raw" > "$OUT/$suite.damo"
    filter < "$OUT/$suite.act4.raw" > "$OUT/$suite.act4"

    sum() { grep -cE '^\[PASS\]' "$1"; }
    d_pass=$(sum "$OUT/$suite.damo"); a_pass=$(sum "$OUT/$suite.act4")
    d_fail=$(grep -cE '^\[FAIL\]' "$OUT/$suite.damo"); a_fail=$(grep -cE '^\[FAIL\]' "$OUT/$suite.act4")

    if [ ! -s "$OUT/$suite.act4" ]; then
        verdict="ACT4 PRODUCED NO TEST OUTPUT"; rc=1
    elif diff -q "$OUT/$suite.damo" "$OUT/$suite.act4" >/dev/null; then
        verdict="identical"
    else
        verdict="DIVERGES"; rc=1
    fi
    printf '%-28s %-10s %-10s %s\n' "$suite" "$d_pass/$d_fail" "$a_pass/$a_fail" "$verdict"

    if [ "$verdict" = "DIVERGES" ]; then
        echo "--- diff (damo -> act4) ---"
        diff "$OUT/$suite.damo" "$OUT/$suite.act4" | head -40
        echo "---------------------------"
    elif [ "$verdict" != "identical" ]; then
        echo "--- act4 tail ---"; tail -25 "$OUT/$suite.act4.raw"; echo "-----------------"
    fi
done
echo
echo "columns are PASS/FAIL counts"
exit $rc
