# 快速开始

## 环境

需要 Git、Make，以及支持 C11 的 GCC 或 Clang。代码使用标准 C 库和数学库，没有包管理器依赖。

- Linux：安装发行版提供的 C 编译工具与 Make。
- macOS：安装 Xcode Command Line Tools（`xcode-select --install`）。
- Windows：建议在 WSL 的 Linux 环境运行；目前 CI 不验证原生 MSVC 构建。

## 下载、运行和测试

```sh
git clone https://github.com/valiidea/fluorescent-fiber-thermometer.git
cd fluorescent-fiber-thermometer
make run
make test
```

`make` 只构建程序；`make run` 构建并运行；`make test` 构建并执行断言测试。产物在被 Git 忽略的 `build/` 中。Makefile 没有 `install` 或 `clean` 目标。

可显式选择编译器和独立输出目录：

```sh
make CC=clang BUILD=build/clang run test
```

更换编译器或编译选项时使用新的 `BUILD` 目录，避免 Make 复用旧产物。

## 如何阅读输出

前两帧示例（小数末位可能因平台而有差异）：

```text
frame   early    late     feature  smoothed  suggestion
    0   0.4260   0.1222   0.3100   0.3100    hold
    1   0.4699   0.1323   0.3054   0.3089    hold
```

| 列 | 含义 |
|---|---|
| frame | 合成帧序号，从 0 开始，共 8 帧 |
| early / late | 去背景后的早期 / 晚期窗口均值 |
| feature | 无量纲相对衰减特征 |
| smoothed | 跨帧加权后的相对特征 |
| suggestion | 对早期窗口均值作比较所得的文字建议 |

反馈依据是 `early`，不是平滑后的衰减特征。所有输出都是合成示意值，不能解释为温度、时间或硬件控制量。

测试成功会输出 `PASS: illustrative analysis, smoothing and textual suggestions`。它只验证演示代码，不验证真实设备性能。

继续阅读 [设计说明](design.md) 与 [代码导读](code-guide.md)。
