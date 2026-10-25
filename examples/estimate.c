/* SPDX-License-Identifier: MIT */
#include "estimator.h"
#include <stdio.h>

int main(void)
{
    est_rng rng;
    est_seed(&rng, 42);
    demo_frame frame;
    if (est_simulate(&rng, 1.0, 0.3, 0.1, 0.01, &frame) != EST_OK) return 1;
    est_config config = est_default_config();
    est_result result;
    est_status status = est_fit(&frame, &config, EST_NLS, &result);
    if (status != EST_OK) {
        fprintf(stderr, "Rejected frame: %s\n", est_status_name(status));
        return 1;
    }
    printf("Synthetic tau=%.6f, signal RMSE=%.6f, R2=%.6f\n",
           result.tau, result.rmse, result.r_squared);
    return 0;
}
