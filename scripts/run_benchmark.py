#!/usr/bin/env python3
"""Run reproducible ILP/BP/BCP benchmark campaigns.

The solver is executed sequentially so CPLEX runs do not compete for CPU or
license tokens. Every run receives its own directory with command, console
logs, solver result and a machine-readable run.json file.
"""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=ROOT / "bin/x64/Release/vne_branch_price.exe")
    parser.add_argument("--substrate", type=Path,
                        default=ROOT / "instances/sub-20.txt")
    parser.add_argument("--requests-folder", type=Path,
                        default=ROOT / "instances/r-250-0-50-20-10-5-25")
    parser.add_argument("--methods", nargs="+", choices=("ilp", "bp", "bcp"),
                        default=("ilp", "bp", "bcp"))
    parser.add_argument("--request-counts", nargs="+", type=int,
                        default=(1, 2, 3, 4, 5))
    parser.add_argument("--time-limits", nargs="+", type=float, default=(120.0,),
                        help="Default is 120 seconds; e.g. 60 300 900 3600")
    parser.add_argument("--repetitions", type=int, default=1)
    parser.add_argument("--heuristic-time-limit", type=float, default=2.0)
    parser.add_argument("--restricted-mip-time-limit", type=float, default=2.0)
    parser.add_argument("--root-only", action="store_true",
                        help="Run only the root node for BP and BCP")
    parser.add_argument("--output-root", type=Path,
                        default=ROOT / "benchmark_results")
    parser.add_argument("--campaign-name",
                        default=datetime.now().strftime("campaign_%Y%m%d_%H%M%S"))
    parser.add_argument("--timeout-grace", type=float, default=30.0,
                        help="External watchdog grace after the solver time limit")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def normalized(path: Path) -> Path:
    return path if path.is_absolute() else (ROOT / path).resolve()


def parse_summary(result_file: Path) -> dict[str, object]:
    if not result_file.exists():
        return {}
    inside = False
    summary: dict[str, object] = {}
    for raw_line in result_file.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()
        if line == "BEGIN_SUMMARY":
            inside = True
            continue
        if line == "END_SUMMARY":
            break
        if inside and "=" in line:
            key, value = line.split("=", 1)
            value = value.strip()
            try:
                summary[key.strip()] = float(value)
            except ValueError:
                summary[key.strip()] = value
    return summary


def write_manifest(campaign: Path, metadata: dict, runs: list[dict]) -> None:
    payload = dict(metadata)
    payload["runs"] = runs
    (campaign / "manifest.json").write_text(
        json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")


def write_csv(campaign: Path, runs: list[dict]) -> None:
    fields = [
        "method", "requests", "time_limit_seconds", "repetition", "root_only",
        "return_code", "watchdog_timeout", "status", "elapsed_seconds",
        "objective", "lower_bound", "gap_percent", "nodes", "cg_iterations",
        "generated_columns", "duplicate_columns", "directory",
    ]
    with (campaign / "summary.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for run in runs:
            summary = run.get("summary", {})
            writer.writerow({
                "method": run["method"],
                "requests": run["requests"],
                "time_limit_seconds": run["time_limit_seconds"],
                "repetition": run["repetition"],
                "root_only": run["root_only"],
                "return_code": run["return_code"],
                "watchdog_timeout": run["watchdog_timeout"],
                "status": summary.get("status", "missing_summary"),
                "elapsed_seconds": summary.get("elapsed_seconds", run["wall_seconds"]),
                "objective": summary.get("objective", ""),
                "lower_bound": summary.get("lower_bound", ""),
                "gap_percent": summary.get("gap_percent", ""),
                "nodes": summary.get("nodes", ""),
                "cg_iterations": summary.get("cg_iterations", ""),
                "generated_columns": summary.get("generated_columns", ""),
                "duplicate_columns": summary.get("duplicate_columns", ""),
                "directory": run["directory"],
            })


def main() -> int:
    args = parse_args()
    exe = normalized(args.exe)
    substrate = normalized(args.substrate)
    requests_folder = normalized(args.requests_folder)
    output_root = normalized(args.output_root)

    if not exe.is_file():
        raise SystemExit(f"Executable not found: {exe}")
    if not substrate.is_file():
        raise SystemExit(f"Substrate not found: {substrate}")
    if not requests_folder.is_dir():
        raise SystemExit(f"Requests folder not found: {requests_folder}")
    if any(count <= 0 for count in args.request_counts):
        raise SystemExit("Request counts must be positive")
    for count in args.request_counts:
        missing = [requests_folder / f"req{i}.txt" for i in range(count)
                   if not (requests_folder / f"req{i}.txt").is_file()]
        if missing:
            raise SystemExit(f"Missing request file: {missing[0]}")
    if any(limit <= 0 for limit in args.time_limits):
        raise SystemExit("Time limits must be positive")

    campaign = output_root / args.campaign_name
    campaign.mkdir(parents=True, exist_ok=False)
    metadata = {
        "created_at": datetime.now().astimezone().isoformat(),
        "executable": str(exe),
        "substrate": str(substrate),
        "requests_folder": str(requests_folder),
        "methods": list(args.methods),
        "request_counts": list(args.request_counts),
        "time_limits": list(args.time_limits),
        "repetitions": args.repetitions,
        "heuristic_time_limit": args.heuristic_time_limit,
        "restricted_mip_time_limit": args.restricted_mip_time_limit,
        "root_only": args.root_only,
    }
    runs: list[dict] = []
    write_manifest(campaign, metadata, runs)

    total = (len(args.methods) * len(args.request_counts) *
             len(args.time_limits) * args.repetitions)
    current = 0
    for method in args.methods:
        for request_count in args.request_counts:
            for time_limit in args.time_limits:
                for repetition in range(1, args.repetitions + 1):
                    current += 1
                    time_label = f"time_{time_limit:g}s".replace(".", "p")
                    run_dir = (campaign / method / f"requests_{request_count:03d}" /
                               time_label / f"run_{repetition:03d}")
                    run_dir.mkdir(parents=True)
                    result_file = run_dir / "result.txt"
                    command = [
                        str(exe), method, str(substrate), str(requests_folder),
                        str(request_count), str(result_file), "--time-limit",
                        str(time_limit), "--heuristic-time-limit",
                        str(args.heuristic_time_limit),
                        "--restricted-mip-time-limit",
                        str(args.restricted_mip_time_limit),
                    ]
                    root_only = args.root_only and method in ("bp", "bcp")
                    if root_only:
                        command.append("--root-only")
                    (run_dir / "command.json").write_text(
                        json.dumps(command, indent=2, ensure_ascii=False), encoding="utf-8")
                    print(f"[{current}/{total}] {method.upper()} | requests={request_count} "
                          f"| limit={time_limit:g}s | run={repetition}", flush=True)

                    started = time.monotonic()
                    return_code = 0
                    watchdog_timeout = False
                    stdout = ""
                    stderr = ""
                    if not args.dry_run:
                        try:
                            completed = subprocess.run(
                                command, cwd=ROOT, capture_output=True, text=True,
                                encoding="utf-8", errors="replace",
                                timeout=time_limit + args.timeout_grace,
                                check=False,
                            )
                            return_code = completed.returncode
                            stdout = completed.stdout
                            stderr = completed.stderr
                        except subprocess.TimeoutExpired as error:
                            watchdog_timeout = True
                            return_code = -1
                            stdout = error.stdout or ""
                            stderr = error.stderr or ""
                            if isinstance(stdout, bytes):
                                stdout = stdout.decode("utf-8", errors="replace")
                            if isinstance(stderr, bytes):
                                stderr = stderr.decode("utf-8", errors="replace")
                    wall_seconds = time.monotonic() - started
                    (run_dir / "stdout.txt").write_text(stdout, encoding="utf-8")
                    (run_dir / "stderr.txt").write_text(stderr, encoding="utf-8")
                    summary = parse_summary(result_file)
                    run = {
                        "method": method,
                        "requests": request_count,
                        "time_limit_seconds": time_limit,
                        "repetition": repetition,
                        "root_only": root_only,
                        "return_code": return_code,
                        "watchdog_timeout": watchdog_timeout,
                        "wall_seconds": wall_seconds,
                        "directory": str(run_dir.relative_to(campaign)),
                        "summary": summary,
                    }
                    (run_dir / "run.json").write_text(
                        json.dumps(run, indent=2, ensure_ascii=False), encoding="utf-8")
                    runs.append(run)
                    write_manifest(campaign, metadata, runs)
                    write_csv(campaign, runs)

    print(f"Campaign saved to: {campaign}")
    print(f"Generate the report with: python scripts/generate_report.py \"{campaign}\"")
    return 0


if __name__ == "__main__":
    sys.exit(main())
