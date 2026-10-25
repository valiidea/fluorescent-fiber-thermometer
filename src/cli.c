/* SPDX-License-Identifier: MIT */
#include "estimator.h"
#include "frame_io.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    puts("thermo: synthetic, dimensionless fluorescence decay experiment toolkit\n"
         "  thermo simulate [--frames N] [--seed N] [--tau X] [--amplitude X]\n"
         "                  [--background X] [--noise X] > frames.csv\n"
         "  thermo analyze --input PATH|- [--method window|log|nls] [--alpha X]\n"
         "                 [--early N] [--late N] [--width N]\n"
         "                 [--tau-min X] [--tau-max X] > estimates.csv\n"
         "  thermo benchmark [--trials N] [--seed N] > benchmark.csv\n"
         "No device access, calibration or temperature output. Errors go to stderr.");
}

static int number(const char *text, double *out)
{
    char *end;
    errno = 0;
    *out = strtod(text, &end);
    return end != text && !*end && !errno && isfinite(*out);
}

static int integer(const char *text, unsigned *out)
{
    char *end;
    if (*text < '0' || *text > '9') return 0;
    errno = 0;
    unsigned long n = strtoul(text, &end, 10);
    if (errno || *end || n > UINT_MAX) return 0;
    *out = (unsigned)n;
    return 1;
}

static int simulate(int argc, char **argv)
{
    unsigned frames = 100, seed = 42;
    double tau = 0.3, amplitude = 1.0, background = 0.1, noise = 0.02;
    for (int i = 2; i < argc; i += 2) {
        if (i + 1 == argc) return 2;
        const char *key = argv[i], *value = argv[i + 1];
        if (!strcmp(key, "--frames")) { if (!integer(value, &frames)) return 2; }
        else if (!strcmp(key, "--seed")) { if (!integer(value, &seed)) return 2; }
        else if (!strcmp(key, "--tau")) { if (!number(value, &tau)) return 2; }
        else if (!strcmp(key, "--amplitude")) { if (!number(value, &amplitude)) return 2; }
        else if (!strcmp(key, "--background")) { if (!number(value, &background)) return 2; }
        else if (!strcmp(key, "--noise")) { if (!number(value, &noise)) return 2; }
        else return 2;
    }
    if (!frames || frames > 1000000 || tau <= 0 || amplitude <= 0 || noise < 0) return 2;
    est_rng rng;
    est_seed(&rng, seed);
    demo_frame frame;
    fprintf(stderr, "synthetic: seed=%u frames=%u tau=%.17g amplitude=%.17g background=%.17g sigma=%.17g\n",
            seed, frames, tau, amplitude, background, noise);
    if (!frame_write_header(stdout)) return 1;
    for (unsigned i = 0; i < frames; ++i) {
        est_status status = est_simulate(&rng, amplitude, tau, background, noise, &frame);
        if (status != EST_OK) {
            fprintf(stderr, "frame %u: %s\n", i, est_status_name(status));
            return 1;
        }
        if (!frame_write(stdout, i, &frame)) return 1;
    }
    return 0;
}

static int analyze(int argc, char **argv)
{
    const char *path = NULL;
    est_method method = EST_NLS;
    est_config config = est_default_config();
    double alpha = 0.25;
    for (int i = 2; i < argc; i += 2) {
        if (i + 1 == argc) return 2;
        const char *key = argv[i], *value = argv[i + 1];
        if (!strcmp(key, "--input")) path = value;
        else if (!strcmp(key, "--method")) {
            if (!strcmp(value, "window")) method = EST_WINDOW;
            else if (!strcmp(value, "log")) method = EST_LOG_LINEAR;
            else if (!strcmp(value, "nls")) method = EST_NLS;
            else return 2;
        } else if (!strcmp(key, "--alpha")) { if (!number(value, &alpha)) return 2; }
        else if (!strcmp(key, "--tau-min")) { if (!number(value, &config.tau_min)) return 2; }
        else if (!strcmp(key, "--tau-max")) { if (!number(value, &config.tau_max)) return 2; }
        else if (!strcmp(key, "--early") || !strcmp(key, "--late") || !strcmp(key, "--width")) {
            unsigned index;
            if (!integer(value, &index)) return 2;
            if (!strcmp(key, "--early")) config.windows.early_start = index;
            else if (!strcmp(key, "--late")) config.windows.late_start = index;
            else config.windows.width = index;
        } else return 2;
    }
    if (!path || alpha <= 0.0 || alpha > 1.0) return 2;
    demo_frame probe;
    demo_generate(0, &probe);
    est_result result;
    if (est_fit(&probe, &config, method, &result) == EST_ARGUMENT) return 2;
    FILE *stream = !strcmp(path, "-") ? stdin : fopen(path, "r");
    if (!stream) { perror(path); return 1; }
    int exit_status = 1;
    if (!frame_read_header(stream)) {
        fprintf(stderr, "Invalid CSV header: expected frame,background,s0,...,s31\n");
        goto done;
    }
    puts("frame,method,status,tau,amplitude,rmse,r_squared,iterations,smoothed");
    demo_filter filter = {0};
    unsigned id, previous = 0;
    size_t rows = 0, rejected = 0;
    demo_frame frame;
    int read_status;
    while ((read_status = frame_read(stream, &id, &frame)) == 1) {
        if (rows && id <= previous) {
            fprintf(stderr, "CSV line %zu: frame IDs must strictly increase\n", rows + 2);
            goto done;
        }
        previous = id;
        ++rows;
        est_status status = est_fit(&frame, &config, method, &result);
        printf("%u,%s,%s,", id, est_method_name(method), est_status_name(status));
        if (status == EST_OK) {
            if (!demo_smooth(&filter, result.tau, alpha)) goto done;
            printf("%.17g,%.17g,%.17g,%.17g,%u,%.17g\n", result.tau,
                   result.amplitude, result.rmse, result.r_squared,
                   result.iterations, filter.value);
        } else {
            /* Never report held filter state as a valid current observation. */
            puts(",,,,,");
            filter = (demo_filter){0};
            ++rejected;
        }
    }
    if (read_status < 0 || !rows) {
        fprintf(stderr, "CSV line %zu: malformed, nonfinite, oversized or empty input\n", rows + 2);
        goto done;
    }
    fprintf(stderr, "frames=%zu accepted=%zu rejected=%zu; synthetic/unitless features\n",
            rows, rows - rejected, rejected);
    exit_status = rejected == rows ? 3 : 0;
done:
    if (stream != stdin && fclose(stream) != 0) exit_status = 1;
    return exit_status;
}

static void benchmark_row(const char *scenario, unsigned trial, unsigned seed,
                          double tau, double amplitude, double ratio, const demo_frame *frame)
{
    est_config config = est_default_config();
    for (est_method method = EST_WINDOW; method <= EST_NLS; ++method) {
        est_result result;
        est_status status = est_fit(frame, &config, method, &result);
        printf("%s,%u,%u,%s,%.17g,%.17g,%.17g,%s,", scenario, trial, seed,
               est_method_name(method), tau, amplitude, ratio, est_status_name(status));
        if (status == EST_OK)
            printf("%.17g,%.17g,%u\n", result.tau, 100.0 * (result.tau / tau - 1.0), result.iterations);
        else puts(",,");
    }
}

static int benchmark(int argc, char **argv)
{
    unsigned trials = 128, seed = 42;
    for (int i = 2; i < argc; i += 2) {
        if (i + 1 == argc) return 2;
        if (!strcmp(argv[i], "--trials")) { if (!integer(argv[i + 1], &trials)) return 2; }
        else if (!strcmp(argv[i], "--seed")) { if (!integer(argv[i + 1], &seed)) return 2; }
        else return 2;
    }
    if (!trials || trials > 10000) return 2;
    const double taus[] = {0.12, 0.3, 0.75};
    const double amplitudes[] = {0.5, 1.0, 2.0};
    const double ratios[] = {0.0, 0.01, 0.03, 0.1};
    est_rng rng;
    est_seed(&rng, seed);
    puts("scenario,trial,seed,method,true_tau,amplitude,noise_ratio,status,tau,error_percent,iterations");
    for (size_t t = 0; t < 3; ++t)
        for (size_t a = 0; a < 3; ++a)
            for (size_t n = 0; n < 4; ++n)
                for (unsigned trial = 0; trial < trials; ++trial) {
                    demo_frame frame;
                    if (est_simulate(&rng, amplitudes[a], taus[t], 0.1,
                                     ratios[n] * amplitudes[a], &frame) != EST_OK) return 1;
                    benchmark_row("gaussian", trial, seed, taus[t], amplitudes[a], ratios[n], &frame);
                }
    const char *scenarios[] = {"background_low", "background_high", "spike", "clipped"};
    for (size_t s = 0; s < 4; ++s)
        for (unsigned trial = 0; trial < trials; ++trial) {
            demo_frame frame;
            if (est_simulate(&rng, 1.0, 0.3, 0.1, 0.01, &frame) != EST_OK) return 1;
            if (s == 0) frame.background -= 0.02;
            if (s == 1) frame.background += 0.02;
            if (s == 2) frame.samples[12] += 0.5;
            if (s == 3)
                for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i)
                    if (frame.samples[i] > 0.65) frame.samples[i] = 0.65;
            benchmark_row(scenarios[s], trial, seed, 0.3, 1.0, 0.01, &frame);
        }
    fprintf(stderr, "benchmark: seed=%u trials_per_cell=%u frames=%u method_evaluations=%u\n",
            seed, trials, trials * 40, trials * 120);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--help")) { usage(); return 0; }
    int result = 2;
    if (argc > 1) {
        if (!strcmp(argv[1], "simulate")) result = simulate(argc, argv);
        else if (!strcmp(argv[1], "analyze")) result = analyze(argc, argv);
        else if (!strcmp(argv[1], "benchmark")) result = benchmark(argc, argv);
    }
    if (result == 2) fprintf(stderr, "Invalid arguments. Run thermo --help for usage.\n");
    if (fflush(stdout) == EOF || ferror(stdout)) {
        fprintf(stderr, "Output write failed\n");
        return 1;
    }
    return result;
}
