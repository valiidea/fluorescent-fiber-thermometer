/* SPDX-License-Identifier: MIT */
#include "estimator.h"
#include <float.h>
#include <math.h>

est_config est_default_config(void)
{
    return (est_config){{4, 16, 4}, 0.03, 2.0};
}

const char *est_status_name(est_status status)
{
    switch (status) {
    case EST_OK: return "ok";
    case EST_ARGUMENT: return "invalid_argument";
    case EST_NONFINITE: return "nonfinite_input";
    case EST_NONPOSITIVE: return "nonpositive_signal";
    case EST_NOT_DECAYING: return "not_decaying";
    case EST_BOUNDARY: return "tau_out_of_bounds";
    default: return "numeric_failure";
    }
}

const char *est_method_name(est_method method)
{
    switch (method) {
    case EST_WINDOW: return "window";
    case EST_LOG_LINEAR: return "log";
    case EST_NLS: return "nls";
    default: return "unknown";
    }
}

/* Profile out amplitude analytically: A = sum(y*g)/sum(g*g).
 * y is scaled once to avoid overflow and preserve scale invariance. */
static double objective(const double *y, size_t start, size_t count,
                        double tau, double *amplitude)
{
    double g[DEMO_SAMPLE_COUNT], yg = 0.0, gg = 0.0, sse = 0.0;
    for (size_t j = 0; j < count; ++j) {
        /* Local amplitude at the first fitted point avoids exp underflow. */
        g[j] = exp(-((double)j / (DEMO_SAMPLE_COUNT - 1U)) / tau);
        yg += y[start + j] * g[j];
        gg += g[j] * g[j];
    }
    *amplitude = yg / gg;
    if (*amplitude <= 0.0) return DBL_MAX;
    for (size_t j = 0; j < count; ++j) {
        double error = y[start + j] - *amplitude * g[j];
        sse += error * error;
    }
    return sse;
}

est_status est_fit(const demo_frame *frame, const est_config *config,
                   est_method method, est_result *result)
{
    if (!result) return EST_ARGUMENT;
    *result = (est_result){0};
    if (!frame || !config || method < EST_WINDOW || method > EST_NLS)
        return EST_ARGUMENT;
    const demo_windows w = config->windows;
    if (!w.width || w.width > DEMO_SAMPLE_COUNT ||
        w.early_start > DEMO_SAMPLE_COUNT - w.width ||
        w.late_start > DEMO_SAMPLE_COUNT - w.width ||
        w.late_start < w.early_start + w.width ||
        !isfinite(config->tau_min) || !isfinite(config->tau_max) ||
        config->tau_min < 1e-4 || config->tau_max > 1e4 ||
        config->tau_min >= config->tau_max) return EST_ARGUMENT;
    if (!isfinite(frame->background)) return EST_NONFINITE;
    size_t start = w.early_start, count = w.late_start + w.width - start;
    double y[DEMO_SAMPLE_COUNT] = {0}, scale = 0.0;
    for (size_t i = start; i < start + count; ++i) {
        if (!isfinite(frame->samples[i])) return EST_NONFINITE;
        y[i] = frame->samples[i] - frame->background;
        if (!isfinite(y[i])) return EST_NUMERIC;
        if (fabs(y[i]) > scale) scale = fabs(y[i]);
    }
    if (scale == 0.0) return EST_NONPOSITIVE;
    double mean = 0.0, covariance = 0.0;
    for (size_t j = 0; j < count; ++j) {
        y[start + j] /= scale;
        mean += y[start + j] / (double)count;
        covariance += ((double)j - (double)(count - 1) / 2.0) * y[start + j];
    }
    if (covariance >= 0.0) return EST_NOT_DECAYING;
    double tau = 0.0;
    unsigned iterations = 0;
    if (method == EST_WINDOW) {
        double early = 0.0, late = 0.0;
        for (size_t j = 0; j < w.width; ++j) {
            early += y[w.early_start + j] / (double)w.width;
            late += y[w.late_start + j] / (double)w.width;
        }
        if (late <= 0.0 || early <= 0.0) return EST_NONPOSITIVE;
        if (early <= late) return EST_NOT_DECAYING;
        tau = ((double)(w.late_start - w.early_start) / (DEMO_SAMPLE_COUNT - 1U)) /
              (log(early) - log(late));
    } else if (method == EST_LOG_LINEAR) {
        double xy = 0.0, xx = 0.0;
        for (size_t j = 0; j < count; ++j) {
            if (y[start + j] <= 0.0) return EST_NONPOSITIVE;
            double x = ((double)j - (double)(count - 1) / 2.0) /
                       (DEMO_SAMPLE_COUNT - 1U);
            xy += x * log(y[start + j]);
            xx += x * x;
        }
        if (xy >= 0.0) return EST_NOT_DECAYING;
        tau = -xx / xy;
    } else {
        /* Coarse global scan followed by bounded golden-section refinement.
         * The scan reduces sensitivity to local minima on imperfect signals;
         * it is not a guarantee of a global optimum for arbitrary inputs. */
        const unsigned grid = 64;
        double lo = log(config->tau_min), hi = log(config->tau_max);
        double step = (hi - lo) / grid, best = DBL_MAX, unused;
        unsigned index = 0;
        for (unsigned i = 0; i <= grid; ++i) {
            double score = objective(y, start, count, exp(lo + i * step), &unused);
            if (score < best) { best = score; index = i; }
        }
        if (best == DBL_MAX) return EST_NONPOSITIVE;
        double left = lo + (index ? index - 1 : 0) * step;
        double right = lo + (index < grid ? index + 1 : grid) * step;
        const double ratio = 0.6180339887498948482;
        double a = right - ratio * (right - left);
        double b = left + ratio * (right - left);
        double fa = objective(y, start, count, exp(a), &unused);
        double fb = objective(y, start, count, exp(b), &unused);
        while (right - left > 1e-10 && iterations < 64) {
            if (fa < fb) {
                right = b; b = a; fb = fa;
                a = right - ratio * (right - left);
                fa = objective(y, start, count, exp(a), &unused);
            } else {
                left = a; a = b; fa = fb;
                b = left + ratio * (right - left);
                fb = objective(y, start, count, exp(b), &unused);
            }
            ++iterations;
        }
        tau = exp((left + right) / 2.0);
    }
    if (!isfinite(tau) || tau <= 0.0) return EST_NUMERIC;
    if (tau <= config->tau_min * (1.0 + 1e-6) ||
        tau >= config->tau_max * (1.0 - 1e-6)) return EST_BOUNDARY;
    double local_amplitude, sse = objective(y, start, count, tau, &local_amplitude);
    if (sse == DBL_MAX) return EST_NONPOSITIVE;
    double sst = 0.0;
    for (size_t j = 0; j < count; ++j) {
        double delta = y[start + j] - mean;
        sst += delta * delta;
    }
    if (sst <= DBL_EPSILON * DBL_EPSILON) return EST_NOT_DECAYING;
    double amplitude = local_amplitude * scale *
                       exp(((double)start / (DEMO_SAMPLE_COUNT - 1U)) / tau);
    double rmse = scale * sqrt(sse / (double)count), r2 = 1.0 - sse / sst;
    if (!isfinite(amplitude) || amplitude <= 0.0 || !isfinite(rmse) || !isfinite(r2))
        return EST_NUMERIC;
    *result = (est_result){tau, amplitude, rmse, r2, iterations};
    return EST_OK;
}
