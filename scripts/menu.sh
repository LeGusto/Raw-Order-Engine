#!/usr/bin/env bash
# Interactive driver for bench.sh and analyze.py.
#
# Usage: scripts/menu.sh
# Pick "Run new benchmark" or "Compare two configs", get prompted for what's needed.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Pick a config directory under bench_results/.
# Sets the global var named in $1 to the chosen path.
pick_config() {
    local prompt="$1"
    local out_var="$2"

    # newest first (by mtime)
    mapfile -t configs < <(
        find bench_results -mindepth 1 -maxdepth 1 -type d -printf '%T@\t%p\n' 2>/dev/null \
            | sort -rn \
            | cut -f2-
    )

    if [[ ${#configs[@]} -eq 0 ]]; then
        echo "no configs found in bench_results/" >&2
        exit 1
    fi

    echo "$prompt"
    select pick in "${configs[@]}"; do
        if [[ -n "$pick" ]]; then
            printf -v "$out_var" '%s' "$pick"
            return
        fi
        echo "invalid choice — pick a number from the list"
    done
}

run_new() {
    local label runs

    while true; do
        read -rp "Label for this run: " label
        if [[ -z "$label" ]]; then
            echo "label can't be empty"
            continue
        fi
        if [[ -d "bench_results/$label" ]] && [[ -n "$(ls -A "bench_results/$label" 2>/dev/null)" ]]; then
            echo "bench_results/$label already has data — pick another label"
            continue
        fi
        break
    done

    read -rp "Number of measured runs [10]: " runs
    runs=${runs:-10}
    if ! [[ "$runs" =~ ^[1-9][0-9]*$ ]]; then
        echo "error: runs must be a positive integer" >&2
        exit 1
    fi

    "$REPO_ROOT/scripts/bench.sh" "$runs" "$label"
}

run_compare() {
    local a b
    pick_config "Pick CONFIG A:" a
    pick_config "Pick CONFIG B:" b

    if [[ "$a" == "$b" ]]; then
        echo "you picked the same config twice — pick two different ones" >&2
        exit 1
    fi

    python "$REPO_ROOT/scripts/analyze.py" --compare "$a" "$b"
}

run_summary() {
    local d
    pick_config "Pick a config to summarize:" d
    python "$REPO_ROOT/scripts/analyze.py" "$d"
}

echo "What would you like to do?"
select main in "Run new benchmark" "Compare two configs" "Summarize one config" "Quit"; do
    case "$REPLY" in
        1) run_new; break ;;
        2) run_compare; break ;;
        3) run_summary; break ;;
        4) exit 0 ;;
        *) echo "invalid choice — pick a number from the list" ;;
    esac
done
