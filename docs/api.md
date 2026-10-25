# C 接口参考

新增估计接口位于 [estimator.h](../include/estimator.h)，共享数据结构位于 [demo.h](../include/demo.h)。`make lib` 构建 `build/libfluorescence.a`；链接时加 `-Iinclude` 与 `-lm`。调用示例见 [estimate.c](../examples/estimate.c)。

## 估计器

```c
est_config est_default_config(void);
est_status est_fit(const demo_frame *frame, const est_config *config,
                   est_method method, est_result *result);
```

`demo_frame` 包含 32 个 `double` 样本及背景。`config.windows` 描述早期起点、晚期起点、共同宽度；`tau_min/tau_max` 默认 0.03/2.0。`method` 可选 `EST_WINDOW`、`EST_LOG_LINEAR`、`EST_NLS`。

约束：窗口宽度非零、不重叠、不越界；`1e-4 <= tau_min < tau_max <= 1e4`。窗口可相邻。拟合区间覆盖从早期起点到晚期窗口末端，只检查这个区间及背景的有限性。该区间全部点用于统一诊断，window 法也会拒绝区间内部非有限值。

成功的 `est_result`：

| 字段 | 含义 |
|---|---|
| tau | 归一化相对衰减参数 |
| amplitude | 模型在 x=0 的幅值；三种方法均使用原始信号域最优幅值 |
| rmse | 拟合区间的信号残差均方根 |
| r_squared | 原始信号域 R²，可以为负 |
| iterations | NLS 局部细化次数，不包括粗搜索；其他方法为 0 |

`result` 非空时函数先清零，失败不得作为有效结果使用。`EST_OK` 的值为 **0**，与旧 `demo_analyze` 的布尔成功返回 **1** 不同；请显式比较 `status == EST_OK`。

| 状态 | 输出字符串 | 典型含义 |
|---|---|---|
| EST_OK | ok | 通过当前数值与边界检查 |
| EST_ARGUMENT | invalid_argument | 指针、方法或配置无效 |
| EST_NONFINITE | nonfinite_input | 背景或拟合点出现 NaN/Inf |
| EST_NONPOSITIVE | nonpositive_signal | 全零、窗口非正、log 非正点或拟合幅值非正 |
| EST_NOT_DECAYING | not_decaying | 非下降趋势、平坦或回归斜率无效 |
| EST_BOUNDARY | tau_out_of_bounds | 估计超界或接近搜索边界 |
| EST_NUMERIC | numeric_failure | 扣背景溢出、结果/外推幅值无法有限表示等 |

多个错误同时存在时返回先发现的错误；状态不是互斥物理故障诊断。`EST_OK` 也不表示模型正确或数据来自真实设备。字符串由 `est_status_name` / `est_method_name` 返回，不应修改或释放。

## 可复现仿真

```c
void est_seed(est_rng *rng, uint32_t seed);
est_status est_simulate(est_rng *rng, double amplitude, double tau,
                        double background, double noise_sigma, demo_frame *frame);
```

调用方持有 RNG；先 `est_seed` 再使用。幅值和 τ 必须有限且 >0，背景有限，噪声标准差有限且 ≥0。成功更新 RNG 与帧；失败保持两者不变。输入验证与帧写入是事务式的，但不保证密码学随机性。所有量无单位。

## 保留的入门 API

- `demo_generate(number, frame)`：生成原始确定性演示帧，frame 必须可写且非空。
- `demo_analyze(frame, windows, result)`：原始双窗口接口，成功 1、失败 0，仅访问两个窗口；结果非空时失败会清零。
- `demo_smooth(filter, input, weight)`：首次有效输入初始化，随后 EWMA；成功 1、失败 0。要求输入、已有状态与结果有限，权重 `(0,1]`；失败保持状态。
- `demo_suggest(level, target, tolerance)`：返回文字建议枚举，有限输入且容差非负，否则 `DEMO_NO_DECISION`。
- `demo_feedback_name(feedback)`：枚举对应 increase / hold / decrease / no decision。

估计器、滤波器和 RNG 不使用全局可变状态。不同实例可独立调用；共享实例需外部同步。API 不提供设备接入、实际时间轴或温度标定。
