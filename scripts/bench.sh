#!/usr/bin/env bash
# Run the workload (warmup + measured) times under a config label.
#
# Usage:
#   scripts/bench.sh [runs] <label>
#
# Runs `WARMUP` extra iterations first and discards them. Then runs `runs`
# more iterations and saves them to bench_results/<label>/run_<i>/.
#
# Warmup runs absorb cold-cache, allocator-warmup, and CPU-frequency-ramp
# effects. They get thrown away.

set -euo pipefail

WARMUP=2
DEFAULT_RUNS=10

usage() {
    cat <<EOF
Usage: $(basename "$0") [runs] <label>

Run the workload <runs> times (default $DEFAULT_RUNS) under <label>.
Plus $WARMUP warmup runs that get discarded before measurement starts.

Examples:
    $(basename "$0") baseline             # ${DEFAULT_RUNS} runs + ${WARMUP} warmup, label=baseline
    $(basename "$0") 20 optimized         # 20 runs + ${WARMUP} warmup, label=optimized
EOF
    exit "${1:-0}"
}

for arg in "$@"; do
    [[ "$arg" == "-h" || "$arg" == "--help" ]] && usage 0
done

if [[ $# -eq 0 ]]; then
    echo "error: missing <label>" >&2
    usage 1
elif [[ $# -eq 1 ]]; then
    RUNS=$DEFAULT_RUNS
    LABEL="$1"
elif [[ $# -eq 2 ]]; then
    RUNS="$1"
    LABEL="$2"
else
    echo "error: too many arguments" >&2
    usage 1
fi

if ! [[ "$RUNS" =~ ^[1-9][0-9]*$ ]]; then
    echo "error: <runs> must be a positive integer, got '$RUNS'" >&2
    usage 1
fi

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

OUT="bench_results/$LABEL"
if [[ -d "$OUT" ]] && [[ -n "$(ls -A "$OUT" 2>/dev/null)" ]]; then
    echo "error: $OUT already has data — pick a different label or remove it" >&2
    exit 1
fi
mkdir -p "$OUT"

if [[ ! -x bin/server ]] || [[ ! -x bin/user ]]; then
    echo "build first: cmake --build build-release" >&2
    exit 1
fi

run_once() {
    local label="$1"   # what to print
    local keep="$2"    # destination dir, or empty to discard

    echo "=== $label ==="

    rm -rf logs/run_*/

    ./bin/server &
    local server_pid=$!
    sleep 0.3

    if ! ./bin/user; then
        kill -INT "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
        echo "user failed during $label" >&2
        exit 1
    fi

    kill -INT "$server_pid"
    wait "$server_pid" 2>/dev/null || true

    local latest
    latest=$(ls -td logs/run_* 2>/dev/null | head -1 || true)
    if [[ -z "$latest" ]]; then
        echo "no run_* dir produced during $label" >&2
        exit 1
    fi

    if [[ -n "$keep" ]]; then
        mv "$latest" "$keep"
    else
        rm -rf "$latest"
    fi
}

for w in $(seq 1 "$WARMUP"); do
    run_once "warmup $w / $WARMUP ($LABEL)" ""
done

for i in $(seq 1 "$RUNS"); do
    run_once "run $i / $RUNS ($LABEL)" "$OUT/run_$i"
done

echo
echo "saved $RUNS runs to $OUT/  (after $WARMUP discarded warmup runs)"
echo "analyze: scripts/analyze.py $OUT"
