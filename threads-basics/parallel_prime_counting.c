#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#define MAX_NUM 20000000
#define NUM_THREADS 4

typedef struct {
    int thread_id;
    int start_val;
    int end_val;
    int local_count;
} ThreadArg;

double get_time_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    
    if (n % 2 == 0 || n % 3 == 0) return false;
    
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

int count_primes_sequential(int max_val) {
    int count = 0;
    for (int i = 1; i <= max_val; i++) {
        if (is_prime(i)) {
            count++;
        }
    }
    return count;
}

void *count_primes_worker(void *arg) {
    ThreadArg *t_arg = (ThreadArg *)arg;
    int count = 0;

    for (int i = t_arg->start_val; i <= t_arg->end_val; i++) {
        if (is_prime(i)) {
            count++;
        }
    }

    t_arg->local_count = count;
    pthread_exit(NULL);
}

int main(void) {
    struct timespec start_time, end_time;
    printf("Counting primes from 1 to %d...\n", MAX_NUM);

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    int seq_count = count_primes_sequential(MAX_NUM);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double seq_duration = get_time_sec(start_time, end_time);
    
    printf("\n[Sequential] Total Primes: %d\n", seq_count);
    printf("[Sequential] Time taken: %.4f seconds\n", seq_duration);

    pthread_t threads[NUM_THREADS];
    ThreadArg thread_args[NUM_THREADS];
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int chunk_size = MAX_NUM / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].thread_id = i;
        
        thread_args[i].start_val = i * chunk_size + 1;
        thread_args[i].end_val = (i == NUM_THREADS - 1) ? MAX_NUM : (i + 1) * chunk_size;
        thread_args[i].local_count = 0;

        if (pthread_create(&threads[i], NULL, count_primes_worker, &thread_args[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    int par_count = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        par_count += thread_args[i].local_count;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double par_duration = get_time_sec(start_time, end_time);

    printf("\n[Parallel]   Total Primes: %d\n", par_count);
    printf("[Parallel]   Time taken: %.4f seconds\n", par_duration);
    
    printf("Speedup: %.2fx\n", seq_duration / par_duration);

    return EXIT_SUCCESS;
}