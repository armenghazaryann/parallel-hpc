#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 3

void *print_message(void *thread_arg) {
    int logical_tid = *((int *)thread_arg);
    
    pthread_t posix_tid = pthread_self();

    printf("Thread %d is running. (POSIX ID: %lu)\n", logical_tid, (unsigned long)posix_tid);
    
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];
    int rc;

    printf("Main thread started. Spawning %d threads...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        
        rc = pthread_create(&threads[i], NULL, print_message, (void *)&thread_args[i]);
        
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create failed with return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        rc = pthread_join(threads[i], NULL);
        
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_join failed with return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
        printf("Main thread joined with Thread %d\n", i);
    }

    printf("All threads completed successfully. Main thread exiting.\n");
    return EXIT_SUCCESS;
}
