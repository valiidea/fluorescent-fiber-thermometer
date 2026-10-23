/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 valiidea
 */

#include "demo.h"
#include <stdio.h>

int main(void)
{
    /* 随意选取的演示窗口，与具体硬件无关。
     * Illustrative window choices, unrelated to a particular board. */
    const demo_windows windows = {4, 16, 4};
    demo_filter filter = {0};

    puts("Fluorescent Fiber Thermometer: conceptual C demonstration");
    puts("Synthetic, dimensionless values. No device access or temperature output.\n");
    puts("frame   early    late     feature  smoothed  suggestion");

    for (unsigned frame_number = 0; frame_number < 8; ++frame_number) {
        demo_frame frame;
        demo_result result;
        demo_generate(frame_number, &frame);
        if (!demo_analyze(&frame, &windows, &result) ||
            !demo_smooth(&filter, result.relative_decay, 0.25)) {
            puts("Example input rejected.");
            return 1;
        }
        demo_feedback suggestion = demo_suggest(result.early, 0.45, 0.03);
        printf("%5u   %.4f   %.4f   %.4f   %.4f    %s\n", frame_number,
               result.early, result.late, result.relative_decay, filter.value,
               demo_feedback_name(suggestion));
    }

    puts("\nTemperature mapping is intentionally left as a conceptual interface.");
    return 0;
}
