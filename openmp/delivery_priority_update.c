#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define NUM_ORDERS 10000
#define NUM_THREADS 4
#define CACHE_LINE_SIZE 64

typedef enum { HIGH, NORMAL, UNASSIGNED } Priority;

typedef struct {
    int order_id;
    float distance_km;
    Priority priority;
} Order;

typedef struct {
    int count;
    char padding[CACHE_LINE_SIZE - sizeof(int)];
} PaddedCounter;

int main() {
    Order* orders = (Order*)malloc(NUM_ORDERS * sizeof(Order));
    if (orders == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    PaddedCounter thread_high_count[NUM_THREADS] = {0};
    
    float threshold = 0.0f;

    srand(time(NULL));
    for (int i = 0; i < NUM_ORDERS; i++) {
        orders[i].order_id = i + 1;
        orders[i].distance_km = (float)(rand() % 500) / 10.0f;
        orders[i].priority = UNASSIGNED;
    }

    omp_set_num_threads(NUM_THREADS);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();

        #pragma omp single
        {
            threshold = 20.0f;
        }
        #pragma omp for nowait
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].distance_km < threshold) {
                orders[i].priority = HIGH;
            } else {
                orders[i].priority = NORMAL;
            }
        }

        #pragma omp barrier

        #pragma omp single
        {
            printf("Thread %d: Priority assignment is finished.\n", tid);
        }

        #pragma omp for nowait
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].priority == HIGH) {
                thread_high_count[tid].count++;
            }
        }

        #pragma omp barrier

        #pragma omp single
        {
            int total_high_priority = 0;            
            for (int t = 0; t < NUM_THREADS; t++) {
                printf("  -> Thread %d found %d HIGH priority orders\n", t, thread_high_count[t].count);
                total_high_priority += thread_high_count[t].count;
            }
            
            printf("TOTAL HIGH PRIORITY ORDERS: %d\n", total_high_priority);
        }
    }

    free(orders);
    return 0;
}
