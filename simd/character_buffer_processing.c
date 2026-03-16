#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <immintrin.h>
#include <string.h>

#define BUFFER_SIZE (256 * 1024 * 1024)
#define NUM_THREADS 4

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// MULTITHREADING

typedef struct {
    char* data;
    size_t start;
    size_t end;
} ThreadArgs;

void* mt_worker(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    for (size_t i = args->start; i < args->end; i++) {
        if (args->data[i] >= 'a' && args->data[i] <= 'z') {
            args->data[i] -= 32;
        }
    }
    return NULL;
}

void process_mt(char* data, size_t size) {
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    size_t chunk_size = size / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].data = data;
        args[i].start = i * chunk_size;
        args[i].end = (i == NUM_THREADS - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, mt_worker, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

// SIMD

void process_simd_core(char* data, size_t start, size_t end) {
    size_t i = start;
    
    __m256i v_a_minus_1 = _mm256_set1_epi8('a' - 1);
    __m256i v_z_plus_1  = _mm256_set1_epi8('z' + 1);
    __m256i v_32        = _mm256_set1_epi8(32);

    while (i + 31 < end) {
        __m256i chunk = _mm256_loadu_si256((__m256i*)&data[i]);

        __m256i mask1 = _mm256_cmpgt_epi8(chunk, v_a_minus_1);
        
        __m256i mask2 = _mm256_cmpgt_epi8(v_z_plus_1, chunk);

        __m256i mask = _mm256_and_si256(mask1, mask2);

        __m256i to_subtract = _mm256_and_si256(mask, v_32);

        chunk = _mm256_sub_epi8(chunk, to_subtract);

        _mm256_storeu_si256((__m256i*)&data[i], chunk);
        
        i += 32;
    }

    for (; i < end; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            data[i] -= 32;
        }
    }
}

void process_simd(char* data, size_t size) {
    process_simd_core(data, 0, size);
}

// SIMD + MULTITHREADING

void* simd_mt_worker(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    process_simd_core(args->data, args->start, args->end);
    return NULL;
}

void process_simd_mt(char* data, size_t size) {
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    size_t chunk_size = size / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].data = data;
        args[i].start = i * chunk_size;
        args[i].end = (i == NUM_THREADS - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, simd_mt_worker, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main() {
    printf("Allocating %d MB buffers...\n", BUFFER_SIZE / (1024*1024));
    
    char* data_mt = (char*)_mm_malloc(BUFFER_SIZE, 32);
    char* data_simd = (char*)_mm_malloc(BUFFER_SIZE, 32);
    char* data_simd_mt = (char*)_mm_malloc(BUFFER_SIZE, 32);

    if (!data_mt || !data_simd || !data_simd_mt) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-= ";
    int charset_len = strlen(charset);
    
    srand(time(NULL));
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        char c = charset[rand() % charset_len];
        data_mt[i] = c;
        data_simd[i] = c;
        data_simd_mt[i] = c;
    }

    double t_start, t_end;

    t_start = get_time();
    process_mt(data_mt, BUFFER_SIZE);
    t_end = get_time();
    double time_mt = t_end - t_start;

    t_start = get_time();
    process_simd(data_simd, BUFFER_SIZE);
    t_end = get_time();
    double time_simd = t_end - t_start;

    t_start = get_time();
    process_simd_mt(data_simd_mt, BUFFER_SIZE);
    t_end = get_time();
    double time_simd_mt = t_end - t_start;

    printf("\nBuffer size: %d MB\n", BUFFER_SIZE / (1024 * 1024));
    printf("Threads used: %d\n\n", NUM_THREADS);
    
    printf("Multithreading time:      %.3f sec\n", time_mt);
    printf("SIMD time:                %.3f sec\n", time_simd);
    printf("SIMD + Multithreading:    %.3f sec\n", time_simd_mt);

    _mm_free(data_mt);
    _mm_free(data_simd);
    _mm_free(data_simd_mt);
    return 0;
}
