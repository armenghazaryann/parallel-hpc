#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_WORKERS 4
#define NUM_STAGES 3

pthread_barrier_t pipeline_barrier;

void* worker_thread(void* arg) {
    int id = *(int*)arg;
    unsigned int seed = id * 100;

    const char* stage_names[NUM_STAGES] = {"Extraction", "Transformation", "Loading"};

    for (int stage = 0; stage < NUM_STAGES; ++stage) {
        int work_time_ms = (rand_r(&seed) % 400) + 100; 
        usleep(work_time_ms * 1000); 
        
        printf("[Worker %d] Completed Stage %d: %s\n", id, stage + 1, stage_names[stage]);

        int status = pthread_barrier_wait(&pipeline_barrier);

        if (status == PTHREAD_BARRIER_SERIAL_THREAD) {
            printf("\n>>> BARRIER: All workers cleared Stage %d (%s) <<<\n\n", 
                   stage + 1, stage_names[stage]);
        }

    }

    printf("[Worker %d] Exiting pipeline.\n", id);
    return NULL;
}

int main() {
    pthread_t workers[NUM_WORKERS];
    int worker_ids[NUM_WORKERS];

    printf("Starting %d-Stage Pipeline with %d Workers...\n\n", NUM_STAGES, NUM_WORKERS);

    if (pthread_barrier_init(&pipeline_barrier, NULL, NUM_WORKERS) != 0) {
        perror("Failed to initialize barrier");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < NUM_WORKERS; ++i) {
        worker_ids[i] = i + 1;
        if (pthread_create(&workers[i], NULL, worker_thread, &worker_ids[i]) != 0) {
            perror("Failed to create thread");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_WORKERS; ++i) {
        pthread_join(workers[i], NULL);
    }

    printf("\nPipeline execution complete. Shutting down.\n");

    pthread_barrier_destroy(&pipeline_barrier);

    return EXIT_SUCCESS;
}
