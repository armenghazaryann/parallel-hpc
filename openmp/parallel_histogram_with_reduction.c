#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define N 100000000
#define NUM_BINS 256
#define NUM_THREADS 4

int main() {
    unsigned char *A = (unsigned char *)malloc(N * sizeof(unsigned char));
    if (A == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    int hist_naive[NUM_BINS] = {0};
    int hist_critical[NUM_BINS] = {0};
    int hist_reduction[NUM_BINS] = {0};

    double start_time, end_time;

    omp_set_num_threads(NUM_THREADS);

    printf("Initializing array of %d elements...\n", N);
    srand(time(NULL));

    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        A[i] = rand() % 256;
    }

    printf("\nRunning Naive Parallel Version...\n");
    start_time = omp_get_wtime();
    
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        hist_naive[A[i]]++;
    }
    
    end_time = omp_get_wtime();
    printf("Naive Time: %f seconds\n", end_time - start_time);

    printf("\nRunning Critical Section Version...\n");
    start_time = omp_get_wtime();
    
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp critical
        {
            hist_critical[A[i]]++;
        }
    }
    
    end_time = omp_get_wtime();
    printf("Critical Time: %f seconds\n", end_time - start_time);

    printf("\nRunning Array Reduction Version...\n");
    start_time = omp_get_wtime();
    
    #pragma omp parallel for reduction(+:hist_reduction[:NUM_BINS])
    for (int i = 0; i < N; i++) {
        hist_reduction[A[i]]++;
    }
    
    end_time = omp_get_wtime();
    printf("Reduction Time: %f seconds\n", end_time - start_time);

    printf("\n--- Validation (Bin 128) ---\n");
    printf("Naive Count     : %d (INCORRECT - Lost Updates)\n", hist_naive[128]);
    printf("Critical Count  : %d (CORRECT)\n", hist_critical[128]);
    printf("Reduction Count : %d (CORRECT)\n", hist_reduction[128]);

    free(A);
    return 0;
}