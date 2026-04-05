#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <math.h>
#include <float.h>

#define N 50000000
#define NUM_THREADS 4

int main() {
    double *A = (double *)malloc(N * sizeof(double));
    if (A == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    printf("Initializing array of %d doubles...\n", N);
    srand((unsigned int)time(NULL));

    for (int i = 0; i < N; i++) {
        A[i] = ((double)rand() / RAND_MAX) * 1000.0;
    }

    double min_diff = DBL_MAX;

    omp_set_num_threads(NUM_THREADS);

    printf("Computing global minimum distance...\n");
    double start_time = omp_get_wtime();

    #pragma omp parallel for reduction(min: min_diff)
    for (int i = 1; i < N; i++) {
        double diff = fabs(A[i] - A[i - 1]);
        if (diff < min_diff) {
            min_diff = diff;
        }
    }

    double end_time = omp_get_wtime();

    printf("\n--- Computation Summary ---\n");
    printf("Global Minimum Difference: %.8f\n", min_diff);
    printf("Time taken: %f seconds\n", end_time - start_time);

    free(A);
    return 0;
}