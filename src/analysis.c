/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 valiidea
 */

#include "demo.h"
#include <math.h>

static double window_mean(const demo_frame *frame, size_t start, size_t width)
{
    double total = 0.0;
    for (size_t i = 0; i < width; ++i) {
        total += frame->samples[start + i] - frame->background;
    }
    return total / (double)width;
}

int demo_analyze(const demo_frame *frame, const demo_windows *windows, demo_result *result)
{
    if (!result) return 0;
    *result = (demo_result){0};
    if (!frame || !windows) return 0;
    if (!isfinite(frame->background) || !windows->width ||
        windows->width > DEMO_SAMPLE_COUNT ||
        windows->early_start > DEMO_SAMPLE_COUNT - windows->width ||
        windows->late_start > DEMO_SAMPLE_COUNT - windows->width ||
        windows->late_start < windows->early_start + windows->width) return 0;

    double early = window_mean(frame, windows->early_start, windows->width);
    double late = window_mean(frame, windows->late_start, windows->width);
    if (!isfinite(early) || !isfinite(late) || early <= late || late <= 0.0) return 0;

    /* 使用归一化位置差，因此结果没有微秒或摄氏度单位。
     * Normalized position difference: the result is neither microseconds nor degrees. */
    double distance = (double)(windows->late_start - windows->early_start) /
                      (DEMO_SAMPLE_COUNT - 1U);
    double feature = distance / (log(early) - log(late));
    if (!isfinite(feature) || feature <= 0.0) return 0;

    *result = (demo_result){early, late, feature};
    return 1;
}
