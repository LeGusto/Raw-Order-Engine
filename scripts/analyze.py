#!/usr/bin/env python3
"""
Analyze raw latency samples produced by LatencyTracker.

Usage:
    analyze.py <run_dir>                 # stats for a single run
    analyze.py <run_dir1> <run_dir2>     # combined stats across runs
    analyze.py --compare <a> <b>         # diff two configs (Mann-Whitney U)
"""

import argparse
import sys
from pathlib import Path

try:
    import numpy as np
except ImportError:
    print("install numpy: pip install numpy", file=sys.stderr)
    sys.exit(1)


def load_bucket(path: Path) -> "np.ndarray":
    if not path.exists() or path.stat().st_size == 0:
        return np.empty(0, dtype=np.uint64)
    return np.loadtxt(path, dtype=np.uint64, ndmin=1)


SKIP_FILES = {"analysis.txt"}


def buckets_in(run_dir: Path) -> dict[str, "np.ndarray"]:
    out: dict[str, "np.ndarray"] = {}
    for f in sorted(run_dir.glob("*.txt")):
        if f.name in SKIP_FILES or f.name.startswith("compare_"):
            continue
        out[f.stem] = load_bucket(f)
    return out


def aggregate_buckets(path: Path) -> dict[str, "np.ndarray"]:
    """Combine bucket samples across run dirs.
    Accepts either a single run dir, or a parent dir containing run_*/."""
    run_dirs = sorted(path.glob("run_*")) or [path]
    combined: dict[str, list["np.ndarray"]] = {}
    for d in run_dirs:
        for name, s in buckets_in(d).items():
            combined.setdefault(name, []).append(s)
    return {k: np.concatenate(v) for k, v in combined.items()}


def fmt_ns(v: float) -> str:
    if v >= 1e6:
        return f"{v/1e6:.2f}ms"
    if v >= 1e3:
        return f"{v/1e3:.1f}µs"
    return f"{int(v)}ns"


def report(label: str, samples: "np.ndarray") -> None:
    if samples.size == 0:
        print(f"=== {label} (no samples) ===")
        return
    print(f"=== {label} (n={samples.size:,}) ===")
    pcts = [50, 95, 99, 99.9, 99.99]
    print(
        f"  min={fmt_ns(samples.min())}  "
        f"avg={fmt_ns(samples.mean())}  "
        f"max={fmt_ns(samples.max())}"
    )
    parts = "  ".join(f"p{p}={fmt_ns(np.percentile(samples, p))}" for p in pcts)
    print(f"  {parts}")


def cmd_summary(run_dirs: list[Path]) -> None:
    combined: dict[str, list["np.ndarray"]] = {}
    for d in run_dirs:
        for name, s in buckets_in(d).items():
            combined.setdefault(name, []).append(s)

    if len(run_dirs) > 1:
        print(
            f"# Combined across {len(run_dirs)} runs: "
            + ", ".join(d.name for d in run_dirs)
        )

    # global aggregate
    all_samples = np.concatenate(
        [s for arrs in combined.values() for s in arrs] or [np.empty(0)]
    )
    report("GLOBAL", all_samples)
    print()

    for name, arrs in sorted(combined.items()):
        report(name, np.concatenate(arrs))
        print()


def cmd_compare(a_dir: Path, b_dir: Path) -> None:
    try:
        from scipy.stats import mannwhitneyu
    except ImportError:
        print("install scipy: pip install scipy", file=sys.stderr)
        sys.exit(1)

    a_buckets = aggregate_buckets(a_dir)
    b_buckets = aggregate_buckets(b_dir)
    keys = sorted(set(a_buckets) | set(b_buckets))

    print(f"# Compare: {a_dir.name}  vs  {b_dir.name}")
    print(
        f"{'bucket':<20} {'a p50':>10} {'b p50':>10} {'Δp50%':>8} "
        f"{'a p99':>10} {'b p99':>10} {'Δp99%':>8}  signif"
    )
    for k in keys:
        a = a_buckets.get(k, np.empty(0))
        b = b_buckets.get(k, np.empty(0))
        if a.size == 0 or b.size == 0:
            print(f"{k:<20}  (missing in one side)")
            continue
        a50, b50 = np.percentile(a, 50), np.percentile(b, 50)
        a99, b99 = np.percentile(a, 99), np.percentile(b, 99)
        d50 = (b50 - a50) / a50 * 100
        d99 = (b99 - a99) / a99 * 100
        _, p = mannwhitneyu(a, b, alternative="two-sided")
        sig = "***" if p < 0.001 else "**" if p < 0.01 else "*" if p < 0.05 else ""
        print(
            f"{k:<20} {fmt_ns(a50):>10} {fmt_ns(b50):>10} {d50:+7.1f}% "
            f"{fmt_ns(a99):>10} {fmt_ns(b99):>10} {d99:+7.1f}%  p={p:.3f} {sig}"
        )


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("paths", nargs="+", type=Path)
    p.add_argument(
        "--compare",
        action="store_true",
        help="compare two configs (each path is a directory of run_* dirs)",
    )
    args = p.parse_args()

    if args.compare:
        if len(args.paths) != 2:
            p.error("--compare needs exactly two directories")
        a, b = args.paths
        out_path = Path("bench_results") / f"compare_{a.name}_vs_{b.name}.txt"
    else:
        # accept either a run_* dir or a parent dir containing run_* dirs
        runs: list[Path] = []
        for path in args.paths:
            if any(path.glob("run_*")):
                runs.extend(sorted(path.glob("run_*")))
            else:
                runs.append(path)
        out_path = args.paths[0] / "analysis.txt"

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w") as f:
        sys.stdout = f
        if args.compare:
            cmd_compare(args.paths[0], args.paths[1])
        else:
            cmd_summary(runs)
        sys.stdout = sys.__stdout__

    print(f"wrote: {out_path.resolve()}")


if __name__ == "__main__":
    main()
