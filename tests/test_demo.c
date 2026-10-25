/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 valiidea
 */

#include "demo.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    demo_frame frame;
    demo_result result;
    demo_windows windows = {4, 16, 4};
    demo_generate(0, &frame);
    assert(demo_analyze(&frame, &windows, &result));
    assert(fabs(result.relative_decay - 0.31) < 1e-12);
    windows.width = 0; assert(!demo_analyze(&frame, &windows, &result));
    windows = (demo_windows){SIZE_MAX, 16, 4};
    assert(!demo_analyze(&frame, &windows, &result));
    windows = (demo_windows){4, 5, 4};
    assert(!demo_analyze(&frame, &windows, &result));
    windows = (demo_windows){4, 16, 4};
    frame.samples[4] = NAN;
    assert(!demo_analyze(&frame, &windows, &result));
    for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) frame.samples[i] = frame.background;
    assert(!demo_analyze(&frame, &windows, &result));

    demo_filter filter = {0};
    assert(demo_smooth(&filter, 1.0, 0.25) && filter.value == 1.0);
    assert(demo_smooth(&filter, 2.0, 0.25) && filter.value == 1.25);
    assert(!demo_smooth(&filter, NAN, 0.25) && filter.value == 1.25);
    assert(!demo_smooth(&filter, 2.0, -0.1));
    filter.value = INFINITY;
    assert(!demo_smooth(&filter, 2.0, 0.5) && isinf(filter.value));
    assert(!demo_smooth(NULL, 1.0, 0.5));
    assert(!demo_smooth(&filter, 1.0, 0.0));
    assert(!demo_smooth(&filter, 1.0, 1.01));
    assert(demo_suggest(0.2, 0.5, 0.1) == DEMO_INCREASE);
    assert(demo_suggest(0.5, 0.5, 0.1) == DEMO_HOLD);
    assert(demo_suggest(0.8, 0.5, 0.1) == DEMO_DECREASE);
    assert(demo_suggest(NAN, 0.5, 0.1) == DEMO_NO_DECISION);
    puts("PASS: illustrative analysis, smoothing and textual suggestions");
    return 0;
}
