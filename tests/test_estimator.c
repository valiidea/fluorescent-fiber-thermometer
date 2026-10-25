/* SPDX-License-Identifier: MIT */
#include "estimator.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)

static demo_frame exact(double amplitude, double tau, double background)
{
    demo_frame frame;
    est_rng rng;
    est_seed(&rng, 123);
    CHECK(est_simulate(&rng, amplitude, tau, background, 0.0, &frame) == EST_OK);
    return frame;
}

static void exact_grid(void)
{
    const double taus[] = {0.04, 0.12, 0.3, 0.75, 1.7};
    const double amplitudes[] = {1e-200, 0.5, 1.0, 1e200};
    est_config config = est_default_config();
    for (size_t t = 0; t < 5; ++t)
        for (size_t a = 0; a < 4; ++a) {
            demo_frame frame = exact(amplitudes[a], taus[t], 0.0);
            for (est_method method = EST_WINDOW; method <= EST_NLS; ++method) {
                est_result result;
                CHECK(est_fit(&frame, &config, method, &result) == EST_OK);
                CHECK(fabs(result.tau / taus[t] - 1.0) < 1e-6);
                CHECK(fabs(result.amplitude / amplitudes[a] - 1.0) < 1e-5);
                CHECK(result.r_squared > 0.999999999);
                CHECK(result.iterations <= 64);
            }
        }
    /* The original double-window estimator remains a compatible baseline. */
    demo_frame frame = exact(1.0, 0.3, 0.1);
    demo_result original;
    est_result result;
    CHECK(demo_analyze(&frame, &config.windows, &original));
    CHECK(est_fit(&frame, &config, EST_WINDOW, &result) == EST_OK);
    CHECK(fabs(result.tau - original.relative_decay) < 1e-12);
    config.windows = (demo_windows){0, 31, 1};
    for (est_method m = EST_WINDOW; m <= EST_NLS; ++m) {
        CHECK(est_fit(&frame, &config, m, &result) == EST_OK);
        CHECK(fabs(result.tau - 0.3) < 1e-6);
    }
}

static void invalid_inputs(void)
{
    demo_frame frame = exact(1.0, 0.3, 0.1);
    est_config config = est_default_config();
    est_result result = {1, 2, 3, 4, 5};
    CHECK(est_fit(NULL, &config, EST_NLS, &result) == EST_ARGUMENT);
    CHECK(result.tau == 0 && result.amplitude == 0 && result.iterations == 0);
    CHECK(est_fit(&frame, NULL, EST_NLS, &result) == EST_ARGUMENT);
    CHECK(est_fit(&frame, &config, EST_NLS, NULL) == EST_ARGUMENT);
    CHECK(est_fit(&frame, &config, (est_method)99, &result) == EST_ARGUMENT);
    const demo_windows invalid[] = {{0, 16, 0}, {SIZE_MAX, 16, 4}, {4, SIZE_MAX, 4},
                                    {4, 5, 4}, {4, 16, 33}, {31, 31, 2}};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        config.windows = invalid[i];
        CHECK(est_fit(&frame, &config, EST_NLS, &result) == EST_ARGUMENT);
    }
    config = est_default_config();
    config.tau_min = NAN;
    CHECK(est_fit(&frame, &config, EST_NLS, &result) == EST_ARGUMENT);
    config = est_default_config();
    config.tau_max = config.tau_min;
    CHECK(est_fit(&frame, &config, EST_NLS, &result) == EST_ARGUMENT);
    config = est_default_config();
    frame.samples[10] = INFINITY;
    for (est_method m = EST_WINDOW; m <= EST_NLS; ++m)
        CHECK(est_fit(&frame, &config, m, &result) == EST_NONFINITE);
    frame = exact(1.0, 0.3, 0.1);
    frame.background = NAN;
    CHECK(est_fit(&frame, &config, EST_NLS, &result) == EST_NONFINITE);
    frame.background = -DBL_MAX;
    frame.samples[4] = DBL_MAX;
    CHECK(est_fit(&frame, &config, EST_NLS, &result) == EST_NUMERIC);
    for (est_method m = EST_WINDOW; m <= EST_NLS; ++m) {
        frame = exact(1.0, 0.005, 0.0);
        CHECK(est_fit(&frame, &config, m, &result) == EST_BOUNDARY);
        frame = exact(1.0, 20.0, 0.0);
        CHECK(est_fit(&frame, &config, m, &result) == EST_BOUNDARY);
        frame = (demo_frame){0};
        CHECK(est_fit(&frame, &config, m, &result) == EST_NONPOSITIVE);
        for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) frame.samples[i] = 1.0;
        CHECK(est_fit(&frame, &config, m, &result) == EST_NOT_DECAYING);
        for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) frame.samples[i] = (double)i;
        CHECK(est_fit(&frame, &config, m, &result) == EST_NOT_DECAYING);
    }
    frame = exact(1.0, 0.3, 0.0);
    frame.samples[19] = -0.01;
    CHECK(est_fit(&frame, &config, EST_LOG_LINEAR, &result) == EST_NONPOSITIVE);
    CHECK(est_fit(&frame, &config, EST_NLS, &result) == EST_OK);
    frame = exact(1.0, 0.3, 0.0);
    frame.samples[31] = NAN; /* Outside the selected interval. */
    CHECK(est_fit(&frame, &config, EST_NLS, &result) == EST_OK);
}

static void noise_and_objective(void)
{
    est_rng a, b;
    est_seed(&a, 0); est_seed(&b, 0);
    demo_frame fa, fb;
    CHECK(est_simulate(&a, 1.0, 0.3, 0.1, 0.02, &fa) == EST_OK);
    CHECK(est_simulate(&b, 1.0, 0.3, 0.1, 0.02, &fb) == EST_OK);
    for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) CHECK(fa.samples[i] == fb.samples[i]);
    uint32_t state = a.state;
    CHECK(est_simulate(&a, -1.0, 0.3, 0.1, 0.02, &fa) == EST_ARGUMENT);
    CHECK(a.state == state);
    CHECK(est_simulate(NULL, 1.0, 0.3, 0.1, 0.02, &fa) == EST_ARGUMENT);
    CHECK(est_simulate(&a, 1.0, 0.3, 0.1, -0.02, &fa) == EST_ARGUMENT);
    CHECK(est_simulate(&a, DBL_MAX, 1.0, DBL_MAX, 0.0, &fa) == EST_NUMERIC);
    CHECK(a.state == state);
    est_config config = est_default_config();
    for (unsigned i = 0; i < 200; ++i) {
        CHECK(est_simulate(&a, 1.0, 0.3, 0.1, 0.02, &fa) == EST_OK);
        est_result window, logfit, nls;
        CHECK(est_fit(&fa, &config, EST_WINDOW, &window) == EST_OK);
        CHECK(est_fit(&fa, &config, EST_LOG_LINEAR, &logfit) == EST_OK);
        CHECK(est_fit(&fa, &config, EST_NLS, &nls) == EST_OK);
        /* NLS minimizes signal-domain residuals; this does not guarantee lower tau error. */
        CHECK(nls.rmse <= window.rmse + 1e-10);
        CHECK(nls.rmse <= logfit.rmse + 1e-10);
        CHECK(nls.tau > config.tau_min && nls.tau < config.tau_max);
    }
}

int main(void)
{
    exact_grid();
    invalid_inputs();
    noise_and_objective();
    printf("PASS: %u estimator assertions (exactness, scaling, bounds, noise, objective)\n", checks);
    return 0;
}
