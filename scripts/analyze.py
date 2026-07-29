#!/usr/bin/env python3
"""
Analyze raw latency samples produced by LatencyTracker.

Usage:
    analyze.py <run_dir>                 # stats for a single run
    analyze.py <run_dir1> <run_dir2>     # combined stats across runs
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
        # compare_*.txt is leftover output from an older comparison mode
        if f.name in SKIP_FILES or f.name.startswith("compare_"):
            continue
        out[f.stem] = load_bucket(f)
    return out


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


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("paths", nargs="+", type=Path)
    args = p.parse_args()

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
        cmd_summary(runs)
        sys.stdout = sys.__stdout__

    print(f"wrote: {out_path.resolve()}")


if __name__ == "__main__":
    main()
