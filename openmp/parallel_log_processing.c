#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define NUM_LOGS 20000
#define NUM_THREADS 4

#define CACHE_LINE_SIZE 64

typedef struct {
    int request_id;
    int user_id;
    int response_time_ms;
} LogEntry;

typedef struct {
    int fast;
    int medium;
    int slow;
    char padding[CACHE_LINE_SIZE - (3 * sizeof(int))]; 
} PaddedCounters;

int main() {
    LogEntry logs[NUM_LOGS];
    PaddedCounters thread_counts[NUM_THREADS] = {0};

    int total_fast = 0, total_medium = 0, total_slow = 0;

    srand(time(NULL));
    omp_set_num_threads(NUM_THREADS);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();

        #pragma omp single
        {
            printf("Thread %d: Initializing %d log entries...\n", tid, NUM_LOGS);
            for (int i = 0; i < NUM_LOGS; i++) {
                logs[i].request_id = i + 1;
                logs[i].user_id = (rand() % 1000) + 1;
                logs[i].response_time_ms = rand() % 500;
            }
        }

        #pragma omp barrier

        #pragma omp for nowait
        for (int i = 0; i < NUM_LOGS; i++) {
            if (logs[i].response_time_ms < 100) {
                thread_counts[tid].fast++;
            } else if (logs[i].response_time_ms <= 300) {
                thread_counts[tid].medium++;
            } else {
                thread_counts[tid].slow++;
            }
        }

        #pragma omp barrier

        #pragma omp single
        {
            printf("Thread %d: Computing summary sequentially...\n", tid);
            
            for (int t = 0; t < NUM_THREADS; t++) {
                total_fast   += thread_counts[t].fast;
                total_medium += thread_counts[t].medium;
                total_slow   += thread_counts[t].slow;
            }

            printf("\n--- Log Processing Summary ---\n");
            printf("Total Logs Processed: %d\n", NUM_LOGS);
            printf("FAST   (<100 ms)    : %d\n", total_fast);
            printf("MEDIUM (100-300 ms) : %d\n", total_medium);
            printf("SLOW   (>300 ms)    : %d\n", total_slow);
        }
    }

    return 0;
}
