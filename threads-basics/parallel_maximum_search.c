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
    int local_max;
} ThreadArg;

double get_time_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int find_max_sequential(int *arr, int size) {
    int max_val = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max_val) {
            max_val = arr[i];
        }
    }
    return max_val;
}

void *find_max_worker(void *arg) {
    ThreadArg *t_arg = (ThreadArg *)arg;
    
    int current_max = t_arg->array_ptr[t_arg->start_idx];

    for (int i = t_arg->start_idx + 1; i < t_arg->end_idx; i++) {
        if (t_arg->array_ptr[i] > current_max) {
            current_max = t_arg->array_ptr[i];
        }
    }

    t_arg->local_max = current_max;

    pthread_exit(NULL); 
}

int main(void) {
    printf("Allocating and initializing array of %d elements...\n", ARRAY_SIZE);
    int *array = malloc(ARRAY_SIZE * sizeof(int));
    if (array == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = rand(); 
    }

    struct timespec start_time, end_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    int seq_max = find_max_sequential(array, ARRAY_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double seq_duration = get_time_sec(start_time, end_time);
    
    printf("\n[Sequential] Global Max: %d\n", seq_max);
    printf("[Sequential] Time taken: %.4f seconds\n", seq_duration);

    pthread_t threads[NUM_THREADS];
    ThreadArg thread_args[NUM_THREADS];
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int chunk_size = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].start_idx = i * chunk_size;
        thread_args[i].end_idx = (i == NUM_THREADS - 1) ? ARRAY_SIZE : (i + 1) * chunk_size;
        thread_args[i].array_ptr = array;
        thread_args[i].local_max = 0;

        if (pthread_create(&threads[i], NULL, find_max_worker, &thread_args[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            free(array);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    int par_max = thread_args[0].local_max;
    for (int i = 1; i < NUM_THREADS; i++) {
        if (thread_args[i].local_max > par_max) {
            par_max = thread_args[i].local_max;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double par_duration = get_time_sec(start_time, end_time);

    printf("\n[Parallel]   Global Max: %d\n", par_max);
    printf("[Parallel]   Time taken: %.4f seconds\n", par_duration);
    printf("Speedup: %.2fx\n", seq_duration / par_duration);

    free(array);
    return EXIT_SUCCESS;
}