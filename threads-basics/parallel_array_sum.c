#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define ARRAY_SIZE 50000000
#define NUM_THREADS 4

typedef struct {
    int thread_id;
    int start_idx;
    int end_idx;
    int *array_ptr;
} ThreadArg;

double get_time_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

long long sum_sequential(int *arr, int size) {
    long long total = 0;
    for (int i = 0; i < size; i++) {
        total += arr[i];
    }
    return total;
}

void *sum_parallel_worker(void *arg) {
    ThreadArg *t_arg = (ThreadArg *)arg;

    long long *partial_sum = malloc(sizeof(long long));
    if (partial_sum == NULL) {
        fprintf(stderr, "Memory allocation failed in thread %d\n", t_arg->thread_id);
        pthread_exit(NULL);
    }
    
    *partial_sum = 0;

    for (int i = t_arg->start_idx; i < t_arg->end_idx; i++) {
        *partial_sum += t_arg->array_ptr[i];
    }

    pthread_exit((void *)partial_sum);
}

int main(void) {
    printf("Allocating and initializing array of %d elements...\n", ARRAY_SIZE);
    int *array = malloc(ARRAY_SIZE * sizeof(int));
    if (array == NULL) {
        fprintf(stderr, "Failed to allocate memory for the array.\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = rand() % 100;
    }

    struct timespec start_time, end_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    long long seq_total = sum_sequential(array, ARRAY_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double seq_duration = get_time_sec(start_time, end_time);
    
    printf("\n[Sequential] Total Sum: %lld\n", seq_total);
    printf("[Sequential] Time taken: %.4f seconds\n", seq_duration);

    pthread_t threads[NUM_THREADS];
    ThreadArg thread_args[NUM_THREADS];
    long long par_total = 0;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int chunk_size = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].start_idx = i * chunk_size;
        
        if (i == NUM_THREADS - 1) {
            thread_args[i].end_idx = ARRAY_SIZE;
        } else {
            thread_args[i].end_idx = (i + 1) * chunk_size;
        }
        thread_args[i].array_ptr = array;

        if (pthread_create(&threads[i], NULL, sum_parallel_worker, &thread_args[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            free(array);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        void *status;
        if (pthread_join(threads[i], &status) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            free(array);
            return EXIT_FAILURE;
        }

        if (status != NULL) {
            long long *partial_sum = (long long *)status;
            par_total += *partial_sum;
            free(partial_sum);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double par_duration = get_time_sec(start_time, end_time);

    printf("\n[Parallel]   Total Sum: %lld\n", par_total);
    printf("[Parallel]   Time taken: %.4f seconds\n", par_duration);
    printf("Speedup: %.2fx\n", seq_duration / par_duration);

    free(array);
    return EXIT_SUCCESS;
}
