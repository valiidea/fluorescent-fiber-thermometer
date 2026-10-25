# 命令行与 CSV 协议

`build/thermo --help` 显示用法。程序不覆盖文件，统一通过 stdout 输出；使用 shell 的 `>` 会按 shell 规则覆盖目标文件，请选择合适的路径。参数使用 `--name value`，不支持 `--name=value`。没有配置文件或隐式设备搜索。

## simulate

```sh
build/thermo simulate --frames 100 --seed 42 --tau 0.3 --amplitude 1 --background 0.1 --noise 0.02 > build/frames.csv
```

| 参数 | 默认值 | 约束 |
|---|---:|---|
| frames | 100 | 1..1,000,000 的整数 |
| seed | 42 | 32 位无符号整数；0 映射到固定非零种子 |
| tau | 0.3 | 有限且 >0 |
| amplitude | 1 | 有限且 >0 |
| background | 0.1 | 有限 |
| noise | 0.02 | 有限且 ≥0，表示绝对标准差 |

数据格式为无引号、逗号分隔的固定 34 列：`frame,background,s0,...,s31`，每行一帧。17 位有效数字用于往返保存 double。帧号从 0 开始，采样位置固定为 `i/31`，不单独记录时间列。

## analyze

```sh
build/thermo analyze --input build/frames.csv --method nls --alpha 0.25 > build/estimates.csv
```

| 参数 | 默认值 | 含义 |
|---|---|---|
| input | 必填 | 文件路径；`-` 读取 stdin |
| method | nls | window / log / nls |
| alpha | 0.25 | EWMA 新值权重，`0 < alpha <= 1` |
| early / late / width | 4 / 16 / 4 | 两窗口起点与等宽；拟合全区间从 early 到 late+width-1 |
| tau-min / tau-max | 0.03 / 2 | 有效估计边界，允许配置范围见接口文档 |

解析器要求精确表头、固定列数、有限数值、递增非重复帧号；支持 LF / CRLF 与末行无换行。拒绝嵌入 NUL、超长行、空数据和多余字段。单行最多 4095 字节（不计 LF），不支持注释、引号、缺失值或通用电子表格 CSV 方言。

输出字段：

```text
frame,method,status,tau,amplitude,rmse,r_squared,iterations,smoothed
```

失败帧保留 ID、method 和 status，其余字段为空。失败会清空 CLI 的平滑历史，下一有效帧直接初始化。算法失败不会终止整个数据文件；文件语法错误会终止。该工具不把合成参数当作温度输出。

## benchmark

```sh
build/thermo benchmark --trials 128 --seed 42 > build/benchmark.csv
python3 tools/report.py build/benchmark.csv --output build/report
```

`trials` 为每个参数组合的重复次数（1..10,000）。固定网格包含 36 个 Gaussian 组合和 4 个压力场景，三种方法共享同一帧输入。输出真值、输入条件、估计、误差、状态与迭代数。网格定义与字段用途见 [实验文档](experiments.md)。

## 退出码

| 码 | 含义 |
|---|---|
| 0 | 完成；analyze 可能包含部分被拒绝帧，需检查每行 status |
| 1 | 文件、CSV 格式、数值生成或输出写入错误；丢弃部分结果 |
| 2 | 命令或参数错误 |
| 3 | analyze 文件有效但全部帧估计失败 |

示例流水线：`build/thermo simulate --frames 10 | build/thermo analyze --input -`。在 shell 脚本中检查管道各阶段状态（支持时启用 `set -o pipefail`），避免忽略上游失败。
