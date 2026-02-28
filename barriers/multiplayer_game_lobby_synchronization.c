#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PLAYERS 4

pthread_barrier_t lobby_barrier;

void* player_thread(void* arg) {
    int id = *(int*)arg;

    unsigned int seed = time(NULL) ^ (id + 1); 
    int prep_time = (rand_r(&seed) % 4) + 1;

    printf("[Lobby] Player %d is getting ready (ETA: %ds)...\n", id, prep_time);
    sleep(prep_time);
    printf("[Lobby] Player %d is READY and waiting at the barrier.\n", id);

    int status = pthread_barrier_wait(&lobby_barrier);

    if (status == PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("\n==========================================\n");
        printf("ALL PLAYERS READY! GAME STARTED!\n");
        printf("==========================================\n\n");
    }
    
    return NULL;
}

int main() {
    pthread_t players[NUM_PLAYERS];
    int player_ids[NUM_PLAYERS];

    printf("Initializing Game Lobby for %d Players...\n\n", NUM_PLAYERS);

    if (pthread_barrier_init(&lobby_barrier, NULL, NUM_PLAYERS) != 0) {
        perror("Failed to initialize barrier");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        player_ids[i] = i + 1;
        if (pthread_create(&players[i], NULL, player_thread, &player_ids[i]) != 0) {
            perror("Failed to create thread");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        pthread_join(players[i], NULL);
    }

    printf("Game session ended. Shutting down lobby.\n");

    pthread_barrier_destroy(&lobby_barrier);

    return EXIT_SUCCESS;
}