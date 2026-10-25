"""Validate benchmark output and produce auditable summaries using only stdlib."""
import argparse
import csv
import hashlib
import json
import math
from collections import defaultdict
from pathlib import Path

METHODS = ('window', 'log', 'nls')
SCENARIOS = ('gaussian', 'background_low', 'background_high', 'spike', 'clipped')
STATUSES = {'ok', 'invalid_argument', 'nonfinite_input', 'nonpositive_signal',
            'not_decaying', 'tau_out_of_bounds', 'numeric_failure'}


def summarize(records):
    errors = [float(row['error_percent']) for row in records if row['status'] == 'ok']
    absolute = sorted(map(abs, errors))
    return {
        'total': len(records), 'accepted': len(errors),
        'rejected_percent': 100 * (len(records) - len(errors)) / len(records),
        'rmse_percent': math.sqrt(sum(e*e for e in errors)/len(errors)) if errors else None,
        'bias_percent': sum(errors)/len(errors) if errors else None,
        'p95_abs_percent': absolute[math.ceil(.95*len(absolute))-1] if errors else None,
    }


def load(path):
    with path.open(newline='') as stream:
        records = list(csv.DictReader(stream))
    if not records:
        raise ValueError('empty benchmark')
    pairs = defaultdict(dict)
    cells = defaultdict(set)
    for row in records:
        if row['method'] not in METHODS or row['status'] not in STATUSES or row['scenario'] not in SCENARIOS:
            raise ValueError('unknown method/status/scenario')
        trial = int(row['trial'])
        if trial < 0:
            raise ValueError('negative trial')
        for key in ('true_tau', 'amplitude', 'noise_ratio'):
            if not math.isfinite(float(row[key])):
                raise ValueError('nonfinite metadata')
        if float(row['true_tau']) <= 0 or float(row['amplitude']) <= 0 or float(row['noise_ratio']) < 0:
            raise ValueError('invalid metadata')
        key = tuple(row[k] for k in ('scenario', 'trial', 'seed', 'true_tau', 'amplitude', 'noise_ratio'))
        if row['method'] in pairs[key]:
            raise ValueError('duplicate method evaluation')
        pairs[key][row['method']] = row
        cells[(row['scenario'], float(row['true_tau']), float(row['amplitude']), float(row['noise_ratio']), row['method'])].add(trial)
        if row['status'] == 'ok':
            tau, error = float(row['tau']), float(row['error_percent'])
            if not math.isfinite(tau) or tau <= 0 or not math.isfinite(error):
                raise ValueError('invalid accepted estimate')
            if not math.isclose(error, 100*(tau/float(row['true_tau'])-1), abs_tol=1e-10):
                raise ValueError('error does not agree with estimate')
        elif row['tau'] or row['error_percent']:
            raise ValueError('rejected estimate must have empty numeric fields')
    if any(set(group) != set(METHODS) for group in pairs.values()):
        raise ValueError('missing paired method')
    seeds = {row['seed'] for row in records}
    trials = max(int(row['trial']) for row in records) + 1
    expected = {('gaussian', tau, amp, noise, method)
                for tau in (.12, .3, .75) for amp in (.5, 1., 2.)
                for noise in (0., .01, .03, .1) for method in METHODS}
    expected.update((scenario, .3, 1., .01, method)
                    for scenario in SCENARIOS[1:] for method in METHODS)
    if len(seeds) != 1 or set(cells) != expected or any(s != set(range(trials)) for s in cells.values()):
        raise ValueError('incomplete experiment grid or mixed seeds')
    return records, pairs, trials, seeds.pop()


def generate(path, output):
    records, pairs, trials, seed = load(path)
    output.mkdir(parents=True, exist_ok=True)
    groups = defaultdict(list)
    for row in records:
        groups[(row['scenario'], float(row['noise_ratio']), row['method'])].append(row)
    summaries = []
    for (scenario, noise, method), data in sorted(groups.items()):
        summaries.append(dict(scenario=scenario, noise_ratio=noise, method=method, **summarize(data)))
    with (output / 'summary.csv').open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=summaries[0])
        writer.writeheader()
        writer.writerows(summaries)
    # Compare on a shared successful subset to avoid survivorship differences.
    paired = {m: [] for m in METHODS}
    for group in pairs.values():
        row = group['nls']
        if row['scenario'] == 'gaussian' and float(row['noise_ratio']) == .03 and all(
                item['status'] == 'ok' for item in group.values()):
            for method in METHODS:
                paired[method].append(group[method])
    paired_stats = {m: summarize(data) for m, data in paired.items() if data}
    metadata = {'input_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                'seed': seed, 'trials_per_cell': trials, 'frames': len(pairs),
                'evaluations': len(records), 'paired_noise_003': paired_stats}
    (output / 'metrics.json').write_text(json.dumps(metadata, indent=2) + '\n')

    def fmt(value):
        return '—' if value is None else f'{value:.4f}'

    lines = ['# 可复现实验结果', '',
             f'- 固定种子：`{seed}`；每个参数组合 {trials} 次独立噪声实现。',
             f'- 共 {len(pairs):,} 帧、{len(records):,} 次方法评估；三个方法共享同一帧输入。',
             f'- 原始 CSV SHA-256：`{metadata["input_sha256"]}`。', '',
             '数据全部为合成数据，τ 与采样位置均无量纲。本报告不代表温度精度或设备性能。', '',
             '## 按场景汇总', '',
             'RMSE / 偏差 / P95 均基于成功估计的相对 τ 误差（百分数）；必须同时阅读拒绝率。',
             'P95 使用 nearest-rank 定义。σ/A 为加性高斯噪声标准差相对于初始幅值的比例，不是尾部 SNR。', '',
             '| 场景 | σ/A | 方法 | 成功/总数 | 拒绝率 % | RMSE % | 偏差 % | P95绝对误差 % |',
             '|---|---:|---|---:|---:|---:|---:|---:|']
    for row in summaries:
        lines.append(f'| {row["scenario"]} | {row["noise_ratio"]:g} | {row["method"]} | '
                     f'{row["accepted"]}/{row["total"]} | {fmt(row["rejected_percent"])} | '
                     f'{fmt(row["rmse_percent"])} | {fmt(row["bias_percent"])} | {fmt(row["p95_abs_percent"])} |')
    lines += ['', '## 同一成功子集比较', '',
              '仅取 Gaussian σ/A=0.03、三种方法都成功的配对样本。该子集会排除较困难样本，不能替代上表全体拒绝率。', '',
              '| 方法 | 配对样本数 | RMSE % |', '|---|---:|---:|']
    for method, stats in paired_stats.items():
        lines.append(f'| {method} | {stats["accepted"]} | {fmt(stats["rmse_percent"])} |')
    lines += ['', '## 实验解释与限制', '',
              '- Gaussian 网格：τ ∈ {0.12, 0.3, 0.75}，A ∈ {0.5, 1, 2}，σ/A ∈ {0, 0.01, 0.03, 0.1}。背景为 0.1。',
              '- 拟合区间为索引 4..19。window 使用 4..7 与 16..19 两段；log / nls 使用区间内全部 16 点。',
              '- 压力场景固定 τ=0.3、A=1、σ=0.01：背景估计偏差 ±0.02、索引 12 加 0.5 尖峰、信号上限裁剪为 0.65。',
              '- window 不读取索引 12 的尖峰进行 τ 估计，因此该特定压力案例可能优于全区间拟合；这不等于普遍抗离群点。',
              '- log 要求全部拟合点扣背景后为正；尾部噪声可导致拒绝。nls 允许负的单点残差，仍有参数边界和趋势检查。',
              '- nls 的最小残差不保证最小参数误差。已知背景、单指数模型与独立同分布噪声是假设；背景偏差、饱和和多指数信号会破坏假设。',
              '- 固定种子支持复现，不构成跨设备验证或置信区间。未声称覆盖全部工况。', '']
    (output / 'results.md').write_text('\n'.join(lines))
    print(f'Validated {len(records)} evaluations; report written to {output}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    generate(args.input, args.output)
