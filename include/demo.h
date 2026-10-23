/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 valiidea
 */

#ifndef DEMO_H
#define DEMO_H

#include <stddef.h>

/* 所有数值均为虚构的无量纲演示量。 / All values are fictional and dimensionless. */
#define DEMO_SAMPLE_COUNT 32U

typedef struct {
    double samples[DEMO_SAMPLE_COUNT];
    double background;
} demo_frame;

typedef struct {
    size_t early_start;
    size_t late_start;
    size_t width;
} demo_windows;

typedef struct {
    double early;
    double late;
    double relative_decay;
} demo_result;

typedef struct {
    int initialized;
    double value;
} demo_filter;

typedef enum {
    DEMO_DECREASE,
    DEMO_HOLD,
    DEMO_INCREASE,
    DEMO_NO_DECISION
} demo_feedback;

/* 生成合成输入，不访问设备。 / Generate synthetic input without accessing a device. */
void demo_generate(unsigned frame_number, demo_frame *frame);

/* 两个示意窗口展示去背景和对数比值，不定义实际采样时序。
 * Two illustrative windows show background removal and a log ratio. */
int demo_analyze(const demo_frame *frame, const demo_windows *windows, demo_result *result);

/* 简单平滑只用于展示“当前值与历史值”的关系。
 * Simple smoothing illustrates the relationship between current and past values. */
int demo_smooth(demo_filter *filter, double input, double weight);

/* 只返回文字建议对应的枚举，不控制任何输出。
 * Return an enum for a textual suggestion; no output is controlled. */
demo_feedback demo_suggest(double level, double target, double tolerance);
const char *demo_feedback_name(demo_feedback feedback);

#endif
