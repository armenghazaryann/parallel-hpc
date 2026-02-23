#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define NUM_THREADS 4
#define ITERATIONS 2000000000LL 

typedef struct {
    int thread_id;
} ThreadArg;

void *cpu_burner_worker(void *arg) {
    ThreadArg *t_arg = (ThreadArg *)arg;
    int t_id = t_arg->thread_id;
    
    int current_cpu = sched_getcpu();
    printf("[Thread %d] INITIALIZED on CPU %d\n", t_id, current_cpu);

    int migrations = 0;
    
    volatile double dummy_math = 0.0;

    for (long long i = 0; i < ITERATIONS; i++) {
        dummy_math += 1.0;

        if (i % 10000000 == 0) {
            int new_cpu = sched_getcpu();
            
            if (new_cpu != current_cpu) {
                printf("[Thread %d] MIGRATED: CPU %d -> CPU %d\n", t_id, current_cpu, new_cpu);
                current_cpu = new_cpu;
                migrations++;
            }
        }
    }

    printf("[Thread %d] FINISHED on CPU %d (Total Migrations: %d)\n", t_id, current_cpu, migrations);
    pthread_exit(NULL);
}

int main(void) {
    printf("Main process started. PID: %d\n", getpid());
    printf("Spawning %d threads to observe scheduling behavior...\n\n", NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    ThreadArg thread_args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].thread_id = i;
        if (pthread_create(&threads[i], NULL, cpu_burner_worker, &thread_args[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nAll threads completed. Exiting.\n");
    return EXIT_SUCCESS;
}