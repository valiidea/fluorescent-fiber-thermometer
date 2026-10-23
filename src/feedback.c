/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 valiidea
 */

#include "demo.h"
#include <math.h>

demo_feedback demo_suggest(double level, double target, double tolerance)
{
    if (!isfinite(level) || !isfinite(target) || !isfinite(tolerance) || tolerance < 0.0)
        return DEMO_NO_DECISION;
    if (level < target - tolerance) return DEMO_INCREASE;
    if (level > target + tolerance) return DEMO_DECREASE;
    return DEMO_HOLD;
}

const char *demo_feedback_name(demo_feedback feedback)
{
    switch (feedback) {
    case DEMO_INCREASE: return "increase";
    case DEMO_DECREASE: return "decrease";
    case DEMO_HOLD: return "hold";
    default: return "no decision";
    }
}
