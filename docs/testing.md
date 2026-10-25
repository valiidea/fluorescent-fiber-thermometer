# 测试与验证策略

## 验证层次

| 层次 | 检查内容 | 入口 |
|---|---|---|
| 原始示例 | 窗口、非有限输入、平滑与文字建议 | `tests/test_demo.c` |
| 算法单元/性质 | 精确曲线、尺度变化、边界、错误状态、确定性 RNG、残差目标 | `tests/test_estimator.c` |
| 端到端 | CLI 参数、CSV、文件/stdin、无效帧复位、退出码 | `tests/test_cli.py` |
| 独立数值参考 | Python 20,001点密集搜索与 C NLS 目标比较 | `tests/test_cli.py` |
| 报告可信性 | 指标手算样例、缺记录、重复、篡改误差 | `tests/test_report.py` |
| 平台 | Linux GCC/Clang、macOS Clang | `.github/workflows/ci.yml` |
| 运行时错误 | ASan / UBSan，包含核心和 CLI 子进程 | CI sanitizers job |

算法测试执行 **1,816 条断言**，包含循环展开后的多组参数检查；这不是1,816个互相独立的测试用例。Python 共9个测试方法，部分使用多个子案例。

## 本地命令

```sh
make all test
make CC=clang BUILD=build/checks \
  CFLAGS='-std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' \
  LDFLAGS='-fsanitize=address,undefined' all test
```

编译测试时使用 `-UNDEBUG`，避免外部优化参数意外禁用断言。消毒器检查不能证明所有输入安全，也不等同于数值准确性证明；两种验证分别执行。工具说明见 [GCC instrumentation](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)。

## 测试设计中的重点

- 精确输入覆盖5种 τ × 4种幅值 × 3种方法，幅值跨 `1e-200..1e200`。
- 非法窗口包含 `SIZE_MAX`，验证加法前先做范围约束，避免整数溢出导致越界。
- 噪声样例验证 NLS 残差不大于两种基线的残差；它不要求每帧 τ 误差都更小。
- 密集网格参考独立使用原始 x 坐标、标准 Python 数学函数，不调用 C 的 objective。
- CSV 测试包含超长行、NUL、非有限数、列数错误、重复 ID、CRLF、无末尾换行。
- CI 还执行完整实验汇总；成功意味着能生成完整网格与报告，不表示获得硬件性能指标。

## 未覆盖内容

尚未进行硬件在环、长时间真实采集、任意字节覆盖引导模糊测试、独立真实数据集验证或正式安全认证。不宣称测试覆盖率百分比。
