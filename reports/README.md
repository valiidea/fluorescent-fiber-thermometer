# 实验快照与审查入口

本目录保存 2026-10-25 实际运行产生的参考结果。全部输入为合成、无量纲信号，不是探头采集数据。

| 文件 | 用途 |
|---|---|
| [results.md](reference/results.md) | 可阅读的完整结果表与实验限制 |
| [benchmark.svg](reference/benchmark.svg) | 误差、拒绝率、背景偏差、尖峰与饱和图表 |
| [benchmark.csv.gz](reference/benchmark.csv.gz) | 15,360 条原始方法评估记录，gzip 压缩 |
| [summary.csv](reference/summary.csv) | 按场景与方法汇总的指标 |
| [metrics.json](reference/metrics.json) | 原始 CSV 哈希与配对成功子集指标 |
| [runtime.csv](reference/runtime.csv) | 各方法20,000次调用的单次主机 CPU 时间测量 |
| [environment.json](reference/environment.json) | 编译器、平台、依赖、命令与源文件 SHA-256 |

## 独立重建报告

```sh
mkdir -p build
python3 -c "import gzip,pathlib; pathlib.Path('build/reference.csv').write_bytes(gzip.decompress(pathlib.Path('reports/reference/benchmark.csv.gz').read_bytes()))"
python3 tools/report.py build/reference.csv --output build/reference-report
```

重新运行算法产生新的原始记录：`make experiment`。参数网格与统计口径见 [实验协议](../docs/experiments.md)。相同源码、平台和参数用于重复性检查；跨 libm 的末位差异可能改变文件哈希，不要求位级一致。

## 运行时间的含义

`runtime.csv` 在 macOS arm64、Apple Clang 21、`-O2` 下运行 `tools/bench_runtime.c`，对128个预生成帧循环调用各方法20,000次。所有调用（包含拒绝）均计入时间；不含生成、解析或绘图。

单次主机结果只展示方法的代价差异，受机器、编译器、调度和计时粒度影响，没有置信区间。它不是 MCU 实测延迟、吞吐承诺或硬件实时性验证。运行 `make benchmark-runtime` 可以得到自己的结果。

## 更新约定

改变估计器、实验网格或统计口径后，重新生成快照并更新环境记录；不要直接修改表格中的指标。`environment.json` 的源文件哈希可用于确认当前源码是否与本次实验一致。
