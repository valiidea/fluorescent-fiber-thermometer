# 快速开始

## 环境

- C11 编译器（GCC / Clang）、Make、标准 C 库与数学库。
- Python 3.9+：用于端到端测试和报告，标准库即可。
- 可选 Matplotlib：只用于生成图，不影响 C 程序运行。
- macOS 可安装 Xcode Command Line Tools；Linux 使用发行版编译工具；Windows 建议 WSL。未验证原生 MSVC。

## 构建与验证

```sh
git clone https://github.com/valiidea/fluorescent-fiber-thermometer.git
cd fluorescent-fiber-thermometer
make all
make test
make run
make example
```

`make all` 生成 `build/demo`、`build/thermo` 和 `build/libfluorescence.a`。`make run` 保留原始八帧演示；`make example` 编译并运行静态库调用示例。只运行 C 测试用 `make test-c`。

指定编译器与输出目录：

```sh
make CC=clang BUILD=build/clang all test
```

更改编译参数后使用新目录或 `make -B`，避免复用旧产物。macOS 的 `gcc` 可能是 Apple Clang，不能据此声称验证了 GNU GCC。

## 生成、分析与比较

```sh
build/thermo simulate --frames 100 --seed 42 --tau 0.3 --noise 0.02 > build/frames.csv
build/thermo analyze --input build/frames.csv --method window > build/window.csv
build/thermo analyze --input build/frames.csv --method log > build/log.csv
build/thermo analyze --input build/frames.csv --method nls > build/nls.csv
make experiment
```

数值结果写到 stdout，运行参数与统计写到 stderr。详细格式、错误码及参数见 [CLI 文档](cli.md)。检查退出码后再使用输出；语法错误时可能已输出部分行。

生成图表：

```sh
python3 -m venv build/plot-env
build/plot-env/bin/python -m pip install -r requirements-plots.txt
build/plot-env/bin/python tools/plot.py build/report/summary.csv --output build/report/benchmark
```

报告位于 `build/report/results.md`，图为 `.svg` 和 `.png`。仓库内也保留一份[已执行快照](../reports/README.md)。

## 最小 API 接入

阅读 [examples/estimate.c](../examples/estimate.c)：初始化 RNG → 生成帧 → 取得默认配置 → `est_fit` → 检查状态 → 使用结果。实际输入方可以直接填充 `demo_frame`，无需调用仿真函数。

## 理解输出

`tau` 是归一化衰减参数；`rmse` 是信号拟合残差；`smoothed` 是对有效 τ 进行 EWMA 后的值。三者均无真实温度单位。`r_squared` 不是测温准确率。原始 demo 中的文字建议也不会输出控制指令。
