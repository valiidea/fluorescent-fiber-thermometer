/* SPDX-License-Identifier: MIT */
#ifndef ESTIMATOR_H
#define ESTIMATOR_H

#include "demo.h"
#include <stdint.h>

typedef enum { EST_WINDOW, EST_LOG_LINEAR, EST_NLS } est_method;
typedef enum {
    EST_OK, EST_ARGUMENT, EST_NONFINITE, EST_NONPOSITIVE,
    EST_NOT_DECAYING, EST_BOUNDARY, EST_NUMERIC
} est_status;

typedef struct {
    demo_windows windows; /* Fit interval spans early_start through late_start + width. */
    double tau_min, tau_max;
} est_config;

typedef struct {
    double tau, amplitude, rmse, r_squared;
    unsigned iterations;
} est_result;

/* All coordinates and signal values are dimensionless. No allocation or I/O. */
est_config est_default_config(void);
est_status est_fit(const demo_frame *frame, const est_config *config,
                   est_method method, est_result *result);
const char *est_status_name(est_status status);
const char *est_method_name(est_method method);

/* Caller owns RNG state. Seed zero is mapped to a fixed nonzero seed. */
typedef struct { uint32_t state; } est_rng;
void est_seed(est_rng *rng, uint32_t seed);
est_status est_simulate(est_rng *rng, double amplitude, double tau,
                        double background, double noise_sigma, demo_frame *frame);

#endif
