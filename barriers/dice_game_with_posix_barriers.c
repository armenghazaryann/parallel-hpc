#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PLAYERS 5
#define NUM_ROUNDS 10

int rolls[NUM_PLAYERS];
int wins[NUM_PLAYERS] = {0};

pthread_barrier_t roll_barrier;
pthread_barrier_t eval_barrier;
pthread_mutex_t rand_mutex;

void* player_thread(void* arg) {
    int id = *(int*)arg;

    for (int r = 0; r < NUM_ROUNDS; ++r) {
        pthread_mutex_lock(&rand_mutex);
        rolls[id] = rand() % 6 + 1;
        pthread_mutex_unlock(&rand_mutex);

        int status = pthread_barrier_wait(&roll_barrier);

        if (status == PTHREAD_BARRIER_SERIAL_THREAD) {
            printf("Round %d results: ", r + 1);
            int max_roll = 0;
            
            for (int i = 0; i < NUM_PLAYERS; ++i) {
                printf("[P%d: %d] ", i, rolls[i]);
                if (rolls[i] > max_roll) {
                    max_roll = rolls[i];
                }
            }

            printf("-> Winner(s): ");
            for (int i = 0; i < NUM_PLAYERS; ++i) {
                if (rolls[i] == max_roll) {
                    wins[i]++;
                    printf("P%d ", i);
                }
            }
            printf("\n");
        }

        pthread_barrier_wait(&eval_barrier);
    }

    return NULL;
}

int main() {
    pthread_t players[NUM_PLAYERS];
    int player_ids[NUM_PLAYERS];

    srand(time(NULL));

    pthread_barrier_init(&roll_barrier, NULL, NUM_PLAYERS);
    pthread_barrier_init(&eval_barrier, NULL, NUM_PLAYERS);
    pthread_mutex_init(&rand_mutex, NULL);

    printf("Starting Dice Game with %d Players for %d Rounds...\n\n", NUM_PLAYERS, NUM_ROUNDS);

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        player_ids[i] = i; 
        if (pthread_create(&players[i], NULL, player_thread, &player_ids[i]) != 0) {
            perror("Failed to create thread");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        pthread_join(players[i], NULL);
    }

    printf("\n--- Final Results ---\n");
    int overall_max_wins = 0;
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        printf("Player %d won %d rounds.\n", i, wins[i]);
        if (wins[i] > overall_max_wins) {
            overall_max_wins = wins[i];
        }
    }

    printf("\n OVERALL WINNER(S): ");
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        if (wins[i] == overall_max_wins) {
            printf("P%d ", i);
        }
    }
    printf("with %d wins! \n", overall_max_wins);

    pthread_barrier_destroy(&roll_barrier);
    pthread_barrier_destroy(&eval_barrier);
    pthread_mutex_destroy(&rand_mutex);

    return EXIT_SUCCESS;
}
