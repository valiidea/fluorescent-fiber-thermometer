"""Render benchmark figures. Optional dependency: matplotlib (see requirements-plots.txt)."""
import argparse
import csv
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def main(summary, output):
    with summary.open(newline='') as stream:
        rows = list(csv.DictReader(stream))
    plt.rcParams.update({'font.size': 10, 'axes.spines.top': False, 'axes.spines.right': False,
                         'svg.hashsalt': 'fluorescent-fiber-thermometer'})
    methods = [('window', 'Two-window', '#0072B2'), ('log', 'Log-linear', '#D55E00'),
               ('nls', 'Profiled NLS', '#009E73')]
    fig, axes = plt.subplots(2, 2, figsize=(12, 8.5), layout='constrained')
    for method, label, color in methods:
        data = sorted((r for r in rows if r['scenario'] == 'gaussian' and r['method'] == method),
                      key=lambda r: float(r['noise_ratio']))
        noise = [100*float(r['noise_ratio']) for r in data]
        axes[0, 0].plot(noise, [float(r['rmse_percent']) for r in data], 'o-', label=label, color=color)
        axes[0, 1].plot(noise, [float(r['rejected_percent']) for r in data], 'o-', label=label, color=color)
    axes[0, 0].set(title='Estimation error on accepted frames', xlabel='Noise standard deviation / amplitude (%)',
                   ylabel='Relative tau RMSE (%)')
    axes[0, 1].set(title='Rejected frames (all samples)', xlabel='Noise standard deviation / amplitude (%)',
                   ylabel='Rejection rate (%)', ylim=(-2, 102))
    scenarios = ['background_low', 'background_high', 'spike', 'clipped']
    for i, (method, label, color) in enumerate(methods):
        data = [next(r for r in rows if r['scenario'] == s and r['method'] == method) for s in scenarios]
        x = [j + (i-1)*.24 for j in range(4)]
        axes[1, 0].bar(x, [float(r['bias_percent']) for r in data], width=.23, color=color, label=label)
        axes[1, 1].bar(x, [float(r['rmse_percent']) for r in data], width=.23, color=color, label=label)
    labels = ['Background\n-0.02', 'Background\n+0.02', 'Spike at\nindex 12', 'Clipping\nat 0.65']
    axes[1, 0].set(title='Sensitivity to model mismatch', ylabel='Relative tau bias (%)', xticks=range(4), xticklabels=labels)
    axes[1, 1].set(title='Stress-case error on accepted frames', ylabel='Relative tau RMSE (%)', xticks=range(4), xticklabels=labels)
    for ax in axes.flat:
        ax.grid(axis='y', alpha=.2)
        ax.set_axisbelow(True)
    axes[0, 0].legend(frameon=False)
    axes[1, 0].axhline(0, color='#555555', linewidth=.8)
    fig.suptitle('Fluorescence decay estimation | synthetic benchmark', fontsize=16, weight='bold')
    fig.supxlabel('Dimensionless tau; known background except stress cases. Read rejection rates alongside errors.', fontsize=10)
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output.with_suffix('.svg'), metadata={'Date': None})
    fig.savefig(output.with_suffix('.png'), dpi=160)
    plt.close(fig)
    print(f'Figures: {output.with_suffix(".svg")} and .png')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('summary', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    main(args.summary, args.output)
