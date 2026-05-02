#!/usr/bin/env bash
# Run the workload N times under a config label.
#
# Usage:
#   scripts/bench.sh <runs> <label>
#
# Each run produces a logs/run_<timestamp>/ directory containing one .txt
# file per bucket with raw nanosecond samples. They get moved into
# bench_results/<label>/run_<i>/.
#
# After running, analyze with:
#   scripts/analyze.py bench_results/<label>
#   scripts/analyze.py --compare bench_results/baseline bench_results/optimized

set -euo pipefail

RUNS=${1:-5}
LABEL=${2:-default}

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

OUT="bench_results/$LABEL"
mkdir -p "$OUT"

if [[ ! -x bin/server ]] || [[ ! -x bin/user ]]; then
    echo "build first: cmake --build build-release" >&2
    exit 1
fi

for i in $(seq 1 "$RUNS"); do
    echo "=== run $i / $RUNS ($LABEL) ==="

    rm -rf logs/run_*/  

    ./bin/server &
    SERVER_PID=$!
    sleep 0.3

    if ! ./bin/user; then
        kill -INT "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        echo "user failed on run $i" >&2
        exit 1
    fi

    kill -INT "$SERVER_PID"
    wait "$SERVER_PID" 2>/dev/null || true

    LATEST=$(ls -td logs/run_* 2>/dev/null | head -1 || true)
    if [[ -z "${LATEST}" ]]; then
        echo "no run_* dir produced" >&2
        exit 1
    fi
    mv "$LATEST" "$OUT/run_$i"
done

echo
echo "saved $RUNS runs to $OUT/"
echo "analyze: scripts/analyze.py $OUT"
