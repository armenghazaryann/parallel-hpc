#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
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

    omp_set_num_threads(NUM_THREADS);

    double start_time, end_time;
    start_time = omp_get_wtime();

    double max_val = -DBL_MAX; 
    
    #pragma omp parallel for reduction(max: max_val)
    for (int i = 0; i < N; i++) {
        if (A[i] > max_val) {
            max_val = A[i];
        }
    }

    double T = 0.8 * max_val;

    double filtered_sum = 0.0;
    long count_filtered = 0;

    #pragma omp parallel for reduction(+: filtered_sum) reduction(+: count_filtered)
    for (int i = 0; i < N; i++) {
        if (A[i] > T) {
            filtered_sum += A[i];
            count_filtered++;
        }
    }

    end_time = omp_get_wtime();

    printf("\n--- Computation Summary ---\n");
    printf("Max Value Found       : %.2f\n", max_val);
    printf("Calculated Threshold  : %.2f\n", T);
    printf("Elements > Threshold  : %ld\n", count_filtered);
    printf("Filtered Sum          : %.2f\n", filtered_sum);
    printf("Total Execution Time  : %f seconds\n", end_time - start_time);

    free(A);
    return 0;
}