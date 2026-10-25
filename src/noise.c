/* SPDX-License-Identifier: MIT */
#include "estimator.h"
#include <math.h>

void est_seed(est_rng *rng, uint32_t seed)
{
    if (rng) rng->state = seed ? seed : UINT32_C(0x9e3779b9);
}

static double uniform(est_rng *rng)
{
    uint32_t x = rng->state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    rng->state = x;
    /* Open interval (0,1), so Box-Muller never evaluates log(0). */
    return ((double)x + 0.5) / 4294967296.0;
}

est_status est_simulate(est_rng *rng, double amplitude, double tau,
                        double background, double noise_sigma, demo_frame *frame)
{
    if (!rng || !frame || !rng->state || !isfinite(amplitude) || amplitude <= 0.0 ||
        !isfinite(tau) || tau <= 0.0 || !isfinite(background) ||
        !isfinite(noise_sigma) || noise_sigma < 0.0) return EST_ARGUMENT;
    demo_frame candidate = {.background = background};
    est_rng next = *rng;
    for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) {
        double u = uniform(&next), v = uniform(&next);
        double noise = sqrt(-2.0 * log(u)) * cos(6.2831853071795864769 * v);
        candidate.samples[i] = background + amplitude *
            exp(-((double)i / (DEMO_SAMPLE_COUNT - 1U)) / tau) + noise_sigma * noise;
        if (!isfinite(candidate.samples[i])) return EST_NUMERIC;
    }
    *frame = candidate;
    *rng = next;
    return EST_OK;
}
