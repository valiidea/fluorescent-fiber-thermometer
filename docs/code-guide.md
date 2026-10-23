# 代码导读

从 `src/main.c` 开始，调用链依次为：

```c
demo_generate(frame_number, &frame);
demo_analyze(&frame, &windows, &result);
demo_smooth(&filter, result.relative_decay, weight);
demo_suggest(result.early, target, tolerance);
```

上面是阅读顺序，完整的错误检查位于 `main.c`。示例数值只用于展示流程，不是产品参数。

## 数据对象

`demo_frame` 保存一帧虚构采样及背景；`demo_windows` 描述窗口范围；`demo_result` 保存两个窗口均值和相对衰减特征。`demo_filter` 是跨帧保留的状态，其余结果可以在每帧重新生成。

这种分工把数据处理与输入来源分开，读者无需理解 MCU 寄存器即可阅读模块之间的关系。

## 分析代码

`analysis.c` 先检查输入指针和窗口范围，随后求均值并验证信号关系，最后计算相对特征。错误路径返回 0，正常路径返回 1。传入有效结果指针时，失败结果会先被清零，调用方仍必须检查返回值。

## 状态与建议

`smoothing.c` 首次收到有效输入时直接初始化状态，后续输入再参与加权。`feedback.c` 将数值比较转为有含义的枚举，打印由调用方负责。两者都不依赖操作系统或硬件接口。

## 简单验证

`make test` 检查虚构指数曲线的相对特征、越界窗口、非有限输入、平滑状态和建议分支。这些检查只说明演示程序本身的行为，不代表任何真实设备测试结果。

如需阅读全部实现，先看接口头文件，再依次阅读 simulation、analysis、smoothing、feedback，最后回到 main。项目没有隐藏的驱动层或额外二进制依赖。
