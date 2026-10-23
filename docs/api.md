# C 接口参考

接口声明位于 [include/demo.h](../include/demo.h)，均使用无量纲演示量。

## 数据结构

| 类型 | 字段与用途 |
|---|---|
| `demo_frame` | `samples[32]` 与固定背景 `background` |
| `demo_windows` | `early_start`、`late_start`、共同宽度 `width`，均为数组索引或数量 |
| `demo_result` | `early`、`late`、`relative_decay` |
| `demo_filter` | `initialized` 标记与历史 `value`；首次使用须零初始化 |
| `demo_feedback` | `DEMO_DECREASE`、`DEMO_HOLD`、`DEMO_INCREASE`、`DEMO_NO_DECISION` |

## 生成数据

`void demo_generate(unsigned frame_number, demo_frame *frame)`

调用方必须传入可写的非空 `frame`；该函数不检查空指针。它生成 32 个合成点，背景为 0.10，归一化位置为 `i / 31`。幅值和衰减随帧序号按正弦 / 余弦变化，同一帧号输入可重复生成结果。

## 分析

`int demo_analyze(const demo_frame *frame, const demo_windows *windows, demo_result *result)`

成功返回 1，失败返回 0。`result` 非空时会先清零；不要在失败后把零值当有效测量结果。

要求窗口非空、不越界，晚期窗口在早期窗口结束之后或紧邻其后，背景与窗口均值有限，且 `early > late > 0`。只读取所选窗口的样本，不逐一验证窗口外的样本。

```text
distance = (late_start - early_start) / 31
relative_decay = distance / (log(early) - log(late))
```

结果也必须有限且大于零。

## 平滑

`int demo_smooth(demo_filter *filter, double input, double weight)`

要求非空滤波器、有限输入以及 `0 < weight <= 1`。首次有效输入直接初始化；随后使用 `(1 - weight) * old + weight * input`。通过输入检查返回 1，否则返回 0 且保持状态。调用方负责保持已有滤波状态有效；实现不对组合运算后的溢出另作检查。

## 文字建议

`demo_feedback demo_suggest(double level, double target, double tolerance)`

输入必须有限，容差不能为负。低于 `target - tolerance` 返回增加，高于 `target + tolerance` 返回减少，包含边界的区间内返回保持。无效输入返回 `DEMO_NO_DECISION`。

`const char *demo_feedback_name(demo_feedback feedback)` 将枚举转换为 `increase`、`hold`、`decrease` 或 `no decision`；未知枚举也返回 `no decision`。返回字符串不应修改或释放。

各模块不分配堆内存、不访问设备。共享同一个 `demo_filter` 的并发调用需要调用方自行同步。
