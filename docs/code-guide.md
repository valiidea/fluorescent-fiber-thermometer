# 代码导读

## 推荐阅读顺序

1. [demo.h](../include/demo.h)：固定帧、窗口和状态的数据模型。
2. [estimator.h](../include/estimator.h)：新估计 API 与状态码。
3. [noise.c](../src/noise.c)：RNG、Box–Muller、带噪单指数生成。
4. [estimator.c](../src/estimator.c)：缩放、三种求解方法、统一质量诊断。
5. [frame_io.c](../src/frame_io.c)：固定 CSV 协议，错误时不提交部分帧。
6. [cli.c](../src/cli.c)：三个命令、帧有效性管理和实验网格。
7. [report.py](../tools/report.py)：数据完整性检查、全体失败率与配对成功子集。
8. [测试目录](../tests/test_estimator.c)：将上述契约转成可运行验证。

`src/main.c`、`simulation.c`、`analysis.c`、`feedback.c` 保留最初的短示例，适合先理解数据传递。新 CLI 独立入口位于 `src/cli.c`，两者分别生成 `demo` 和 `thermo`。

## 值得深入的实现细节

- NLS 的 `objective` 以局部起点定义幅值，用闭式解消去幅值参数。
- 三方法共享归一化信号；极端幅值测试说明缩放的作用。
- `est_simulate` 在临时帧与临时 RNG 上工作，成功后才提交状态。
- `frame_read` 严格限制每行长度和字段数量，失败不覆盖输出帧。
- CLI 在失败帧上清空滤波历史，CSV 数值字段留空，避免旧值被当作新测量。
- 报告校验完整网格和配对记录，再汇总误差；图表不能脱离拒绝率阅读。

完整数学说明见 [算法文档](algorithms.md)，复现入口见 [实验文档](experiments.md)。
