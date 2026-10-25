/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 valiidea
 */

#include "demo.h"
#include <math.h>

int demo_smooth(demo_filter *filter, double input, double weight)
{
    if (!filter || !isfinite(input) || !isfinite(weight) || weight <= 0.0 || weight > 1.0) return 0;
    if (filter->initialized && !isfinite(filter->value)) return 0;
    double next = filter->initialized ? (1.0 - weight) * filter->value + weight * input : input;
    if (!isfinite(next)) return 0;
    filter->value = next;
    filter->initialized = 1;
    return 1;
}
