#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUM_THREADS 4

long long fib_seq(int n) {
    if (n < 2) return n;
    return fib_seq(n - 1) + fib_seq(n - 2);
}

long long fib_parallel(int n) {
    if (n <= 10) {
        return fib_seq(n);
    }

    long long x, y;

    #pragma omp task shared(x)
    x = fib_parallel(n - 1);

    #pragma omp task shared(y)
    y = fib_parallel(n - 2);

    #pragma omp taskwait

    return x + y;
}

int main(int argc, char *argv[]) {
    int num = 35; 
    
    if (argc > 1) {
        num = atoi(argv[1]);
    }

    long long result = 0;
    
    omp_set_num_threads(NUM_THREADS);

    printf("Calculating Fibonacci(%d) using OpenMP tasks...\n", num);
    double start_time = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            result = fib_parallel(num);
        }
    }

    double end_time = omp_get_wtime();

    printf("\n--- Computation Summary ---\n");
    printf("F(%d) = %lld\n", num, result);
    printf("Execution Time: %f seconds\n", end_time - start_time);

    return 0;
}
