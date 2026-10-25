/* SPDX-License-Identifier: MIT */
#include "estimator.h"
#include <stdio.h>
#include <time.h>

int main(void)
{
    demo_frame frames[128];
    est_rng rng;
    est_seed(&rng, 42);
    for (unsigned i = 0; i < 128; ++i)
        if (est_simulate(&rng, 1.0, 0.12 + 0.005 * i, 0.1, 0.01, &frames[i]) != EST_OK) return 1;
    est_config config = est_default_config();
    puts("method,calls,accepted,cpu_seconds,microseconds_per_call,checksum");
    for (est_method m = EST_WINDOW; m <= EST_NLS; ++m) {
        const unsigned calls = 20000;
        unsigned accepted = 0;
        double checksum = 0.0;
        clock_t begin = clock();
        if (begin == (clock_t)-1) return 1;
        for (unsigned i = 0; i < calls; ++i) {
            est_result result;
            if (est_fit(&frames[i % 128], &config, m, &result) == EST_OK) {
                checksum += result.tau;
                ++accepted;
            }
        }
        clock_t end = clock();
        if (end == (clock_t)-1) return 1;
        double seconds = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("%s,%u,%u,%.9g,%.9g,%.17g\n", est_method_name(m), calls, accepted,
               seconds, 1e6 * seconds / calls, checksum);
    }
    return fflush(stdout) == EOF;
}
