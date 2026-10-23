/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 valiidea
 */

#include "demo.h"
#include <math.h>

void demo_generate(unsigned number, demo_frame *frame)
{
    /* 人为变化用于演示，不来自测量或标定数据。
     * Artificial variation for the example, not measured or calibrated data. */
    const double amplitude = 0.75 + 0.10 * sin((double)number);
    const double decay = 0.30 + 0.01 * cos((double)number);
    frame->background = 0.10;

    for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) {
        double position = (double)i / (DEMO_SAMPLE_COUNT - 1U);
        frame->samples[i] = frame->background + amplitude * exp(-position / decay);
    }
}
