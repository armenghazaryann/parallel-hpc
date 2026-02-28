#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_SENSORS 5

double temperatures[NUM_SENSORS];
pthread_barrier_t sync_barrier;

void* sensor_thread(void* arg) {
    int id = *(int*)arg;

    unsigned int seed = time(NULL) ^ (id * 1337); 
    
    usleep((rand_r(&seed) % 500) * 1000); 
    
    double temp = -10.0 + ((double)rand_r(&seed) / RAND_MAX) * 50.0;
    temperatures[id] = temp;
    
    printf("[Sensor %d] Collected temperature: %.2f°C\n", id, temp);

    int status = pthread_barrier_wait(&sync_barrier);

    if (status == PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("\n--- Aggregating Sensor Data ---\n");
        double sum = 0.0;
        
        for (int i = 0; i < NUM_SENSORS; i++) {
            sum += temperatures[i];
        }
        
        double average = sum / NUM_SENSORS;
        printf("Global Average Temperature: %.2f°C\n", average);
        printf("-------------------------------\n");
    }

    return NULL;
}

int main() {
    pthread_t sensors[NUM_SENSORS];
    int sensor_ids[NUM_SENSORS];

    printf("Initializing Weather Station with %d Sensors...\n\n", NUM_SENSORS);

    if (pthread_barrier_init(&sync_barrier, NULL, NUM_SENSORS) != 0) {
        perror("Barrier initialization failed");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
        sensor_ids[i] = i;
        if (pthread_create(&sensors[i], NULL, sensor_thread, &sensor_ids[i]) != 0) {
            perror("Thread creation failed");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
        pthread_join(sensors[i], NULL);
    }

    pthread_barrier_destroy(&sync_barrier);
    printf("\nWeather data collection complete. System shutting down.\n");

    return EXIT_SUCCESS;
}
