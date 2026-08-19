#!/usr/bin/env python3
"""Generate a self-contained HTML comparison from a benchmark campaign."""

from __future__ import annotations

import argparse
import html
import json
import math
import statistics
from collections import defaultdict
from datetime import datetime
from pathlib import Path
from typing import Optional


COLORS = {"ilp": "#7c3aed", "bp": "#0284c7", "bcp": "#059669"}


def number(value: object, digits: int = 3) -> str:
    try:
        numeric = float(value)
    except (TypeError, ValueError):
        return "—"
    if not math.isfinite(numeric):
        return "—"
    return f"{numeric:,.{digits}f}".replace(",", " ")


def numeric(summary: dict, key: str) -> Optional[float]:
    try:
        value = float(summary[key])
        return value if math.isfinite(value) else None
    except (KeyError, TypeError, ValueError):
        return None


def aggregate(runs: list[dict]) -> list[dict]:
    groups: dict[tuple, list[dict]] = defaultdict(list)
    for run in runs:
        if run.get("summary"):
            key = (run["method"], int(run["requests"]),
                   float(run["time_limit_seconds"]), bool(run["root_only"]),
                   int(run.get("summary", {}).get("tree_threads",
                                                   run.get("tree_threads", 0))))
            groups[key].append(run)

    rows = []
    metrics = ("elapsed_seconds", "objective", "lower_bound", "gap_percent",
               "nodes", "cg_iterations", "generated_columns", "duplicate_columns",
               "max_active_workers", "open_nodes_remaining")
    for (method, requests, limit, root_only, tree_threads), group in sorted(groups.items()):
        row = {"method": method, "requests": requests, "time_limit_seconds": limit,
               "root_only": root_only, "tree_threads": tree_threads,
               "repetitions": len(group)}
        for metric in metrics:
            values = [value for run in group
                      if (value := numeric(run["summary"], metric)) is not None]
            row[metric] = statistics.mean(values) if values else None
        statuses = sorted({str(run["summary"].get("status", "unknown")) for run in group})
        row["status"] = ", ".join(statuses)
        rows.append(row)
    return rows


def line_chart(rows: list[dict], metric: str, title: str, suffix: str = "") -> str:
    usable = [row for row in rows if row.get(metric) is not None]
    if not usable:
        return ""
    width, height = 760, 300
    left, right, top, bottom = 64, 24, 34, 48
    values = [float(row[metric]) for row in usable]
    y_max = max(values) or 1.0
    x_values = sorted({int(row["requests"]) for row in usable})
    x_min, x_max = min(x_values), max(x_values)

    def x_pos(value: int) -> float:
        if x_min == x_max:
            return (left + width - right) / 2
        return left + (value - x_min) * (width - left - right) / (x_max - x_min)

    def y_pos(value: float) -> float:
        return top + (1 - value / y_max) * (height - top - bottom)

    chunks = [f'<section class="chart"><h2>{html.escape(title)}</h2>',
              f'<svg viewBox="0 0 {width} {height}" role="img" '
              f'aria-label="{html.escape(title)}">']
    for tick in range(5):
        value = y_max * tick / 4
        y = y_pos(value)
        chunks.append(f'<line class="grid" x1="{left}" y1="{y:.1f}" '
                      f'x2="{width-right}" y2="{y:.1f}"/>')
        chunks.append(f'<text class="axis" x="{left-8}" y="{y+4:.1f}" '
                      f'text-anchor="end">{number(value, 1)}{suffix}</text>')
    for request in x_values:
        x = x_pos(request)
        chunks.append(f'<text class="axis" x="{x:.1f}" y="{height-18}" '
                      f'text-anchor="middle">{request}</text>')

    series: dict[tuple, list[dict]] = defaultdict(list)
    for row in usable:
        series[(row["method"], row["time_limit_seconds"], row["root_only"],
                row["tree_threads"])].append(row)
    legend = []
    for index, ((method, limit, root_only, tree_threads), points) in enumerate(series.items()):
        base = COLORS.get(method, "#475569")
        opacity = max(0.45, 1.0 - index * 0.08)
        points.sort(key=lambda item: item["requests"])
        coordinates = " ".join(
            f'{x_pos(int(point["requests"])):.1f},{y_pos(float(point[metric])):.1f}'
            for point in points)
        chunks.append(f'<polyline points="{coordinates}" fill="none" stroke="{base}" '
                      f'stroke-opacity="{opacity}" stroke-width="3"/>')
        for point in points:
            chunks.append(f'<circle cx="{x_pos(int(point["requests"])):.1f}" '
                          f'cy="{y_pos(float(point[metric])):.1f}" r="4" fill="{base}">'
                          f'<title>{method.upper()} · {point["requests"]} req · '
                          f'{number(point[metric])}{suffix}</title></circle>')
        label = f'{method.upper()} · {limit:g}s' + (' · raiz' if root_only else '')
        if tree_threads:
            label += f' · {tree_threads} threads'
        legend.append(f'<span><i style="background:{base}"></i>{html.escape(label)}</span>')
    chunks.append(f'<text class="axis-title" x="{width/2}" y="{height-3}" '
                  f'text-anchor="middle">Número de requisições</text>')
    chunks.append('</svg><div class="legend">' + "".join(legend) + '</div></section>')
    return "".join(chunks)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaign", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    campaign = args.campaign.resolve()
    manifest_file = campaign / "manifest.json"
    if not manifest_file.is_file():
        raise SystemExit(f"manifest.json not found in {campaign}")
    manifest = json.loads(manifest_file.read_text(encoding="utf-8"))
    runs = manifest.get("runs", [])
    rows = aggregate(runs)
    output = args.output.resolve() if args.output else campaign / "report.html"

    successful = sum(1 for run in runs if run.get("summary"))
    watchdogs = sum(1 for run in runs if run.get("watchdog_timeout"))
    gaps = [float(row["gap_percent"]) for row in rows if row.get("gap_percent") is not None]
    table_rows = []
    for row in rows:
        method = row["method"]
        badge = f'<span class="badge {method}">{method.upper()}</span>'
        table_rows.append(
            "<tr>" +
            f"<td>{badge}</td><td>{row['requests']}</td>" +
            f"<td>{number(row['time_limit_seconds'], 0)} s</td>" +
            f"<td>{'Sim' if row['root_only'] else 'Não'}</td>" +
            f"<td>{row['tree_threads'] or '—'}</td>" +
            f"<td>{number(row['max_active_workers'], 1)}</td>" +
            f"<td>{number(row['open_nodes_remaining'], 1)}</td>" +
            f"<td>{html.escape(row['status'])}</td>" +
            f"<td>{number(row['objective'])}</td><td>{number(row['lower_bound'])}</td>" +
            f"<td>{number(row['gap_percent'], 2)}%</td>" +
            f"<td>{number(row['elapsed_seconds'], 2)} s</td>" +
            f"<td>{number(row['nodes'], 1)}</td><td>{number(row['cg_iterations'], 1)}</td>" +
            f"<td>{number(row['generated_columns'], 1)}</td>" +
            f"<td>{number(row['duplicate_columns'], 1)}</td>" +
            f"<td>{row['repetitions']}</td></tr>"
        )

    created = html.escape(str(manifest.get("created_at", "—")))
    charts = (line_chart(rows, "gap_percent", "Gap médio por tamanho", "%") +
              line_chart(rows, "elapsed_seconds", "Tempo médio por tamanho", "s") +
              line_chart(rows, "generated_columns", "Colunas geradas", ""))
    document = f"""<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>VNE Solver · Benchmark</title>
<style>
:root{{--ink:#172033;--muted:#64748b;--surface:#fff;--bg:#f1f5f9;--line:#dbe3ee}}
*{{box-sizing:border-box}} body{{margin:0;background:var(--bg);color:var(--ink);font:14px Inter,Segoe UI,Arial,sans-serif}}
header{{padding:42px max(24px,calc((100vw - 1280px)/2));color:white;background:linear-gradient(120deg,#172554,#075985 60%,#047857)}}
header h1{{margin:0 0 8px;font-size:34px}} header p{{margin:0;opacity:.82}}
main{{max-width:1280px;margin:auto;padding:24px}} .cards{{display:grid;grid-template-columns:repeat(4,1fr);gap:14px;margin-top:-44px}}
.card,.chart,.table-wrap{{background:var(--surface);border:1px solid var(--line);border-radius:16px;box-shadow:0 8px 28px #0f172a0d}}
.card{{padding:18px}} .card strong{{display:block;font-size:28px;margin-top:8px}} .card span{{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.08em}}
.charts{{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin:22px 0}} .chart{{padding:18px}} .chart:last-child{{grid-column:1/-1}}
h2{{margin:0 0 12px;font-size:18px}} svg{{width:100%;height:auto}} .grid{{stroke:#e2e8f0;stroke-width:1}} .axis{{fill:#64748b;font-size:11px}} .axis-title{{fill:#475569;font-size:12px}}
.legend{{display:flex;gap:14px;flex-wrap:wrap;color:var(--muted);font-size:12px}} .legend i{{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:5px}}
.table-wrap{{overflow:auto;margin-bottom:30px}} table{{width:100%;border-collapse:collapse;white-space:nowrap}} th,td{{padding:12px 14px;text-align:right;border-bottom:1px solid #edf2f7}}
th{{position:sticky;top:0;background:#f8fafc;color:#475569;font-size:11px;text-transform:uppercase}} th:first-child,td:first-child,th:nth-child(8),td:nth-child(8){{text-align:left}}
.badge{{padding:4px 8px;border-radius:999px;color:white;font-weight:700;font-size:11px}} .badge.ilp{{background:#7c3aed}} .badge.bp{{background:#0284c7}} .badge.bcp{{background:#059669}}
footer{{color:var(--muted);padding:0 0 30px}} @media(max-width:800px){{.cards{{grid-template-columns:1fr 1fr}}.charts{{grid-template-columns:1fr}}.chart:last-child{{grid-column:auto}}}}
</style></head><body>
<header><h1>VNE Solver · Benchmark</h1><p>Campanha criada em {created} · relatório gerado em {datetime.now().astimezone().isoformat()}</p></header>
<main><section class="cards">
<div class="card"><span>Execuções</span><strong>{len(runs)}</strong></div>
<div class="card"><span>Com resumo válido</span><strong>{successful}</strong></div>
<div class="card"><span>Gap médio</span><strong>{number(statistics.mean(gaps) if gaps else None, 2)}%</strong></div>
<div class="card"><span>Watchdogs</span><strong>{watchdogs}</strong></div>
</section><section class="charts">{charts}</section>
<div class="table-wrap"><table><thead><tr><th>Método</th><th>Req.</th><th>Limite</th><th>Só raiz</th><th>Threads</th><th>Máx. ativos</th><th>Abertos</th><th>Status</th><th>UB</th><th>LB</th><th>Gap</th><th>Tempo</th><th>Nós</th><th>Iter. GC</th><th>Colunas</th><th>Duplicadas</th><th>Rep.</th></tr></thead>
<tbody>{''.join(table_rows)}</tbody></table></div>
<footer>Relatório autocontido. Dados brutos, logs e comandos permanecem na pasta da campanha.</footer></main></body></html>"""
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(document, encoding="utf-8")
    print(f"Report generated: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
