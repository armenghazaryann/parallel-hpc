#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <immintrin.h>

#define DNA_SIZE (256 * 1024 * 1024)
#define NUM_THREADS 4

size_t global_mt_counts[4] = {0};
pthread_mutex_t mt_mutex = PTHREAD_MUTEX_INITIALIZER;

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline int char_to_idx(char c) {
    if (c == 'A') return 0;
    if (c == 'C') return 1;
    if (c == 'G') return 2;
    return 3;
}

// SCALAR 

void count_scalar(const char* data, size_t size, size_t* counts) {
    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    for (size_t i = 0; i < size; i++) {
        counts[char_to_idx(data[i])]++;
    }
}

// MULTITHREADING

typedef struct {
    const char* data;
    size_t start;
    size_t end;
} ThreadArgsMT;

void* mt_worker(void* arg) {
    ThreadArgsMT* args = (ThreadArgsMT*)arg;
    size_t local_counts[4] = {0};

    for (size_t i = args->start; i < args->end; i++) {
        local_counts[char_to_idx(args->data[i])]++;
    }

    pthread_mutex_lock(&mt_mutex);
    for (int i = 0; i < 4; i++) {
        global_mt_counts[i] += local_counts[i];
    }
    pthread_mutex_unlock(&mt_mutex);

    return NULL;
}

void count_mt(const char* data, size_t size, size_t* counts) {
    pthread_t threads[NUM_THREADS];
    ThreadArgsMT args[NUM_THREADS];
    size_t chunk_size = size / NUM_THREADS;

    global_mt_counts[0] = global_mt_counts[1] = global_mt_counts[2] = global_mt_counts[3] = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].data = data;
        args[i].start = i * chunk_size;
        args[i].end = (i == NUM_THREADS - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, mt_worker, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < 4; i++) {
        counts[i] = global_mt_counts[i];
    }
}

// SIMD

void count_simd_core(const char* data, size_t start, size_t end, size_t* counts) {
    size_t countA = 0, countC = 0, countG = 0, countT = 0;
    size_t i = start;

    __m256i vA = _mm256_set1_epi8('A');
    __m256i vC = _mm256_set1_epi8('C');
    __m256i vG = _mm256_set1_epi8('G');
    __m256i vT = _mm256_set1_epi8('T');
    __m256i zero = _mm256_setzero_si256();

    __m256i totalA = zero, totalC = zero, totalG = zero, totalT = zero;

    while (i + 31 < end) {
        int batch = (end - i) / 32;
        if (batch > 255) batch = 255;

        __m256i localA = zero, localC = zero, localG = zero, localT = zero;

        for (int b = 0; b < batch; b++) {
            __m256i chunk = _mm256_loadu_si256((const __m256i*)&data[i]);
            
            localA = _mm256_sub_epi8(localA, _mm256_cmpeq_epi8(chunk, vA));
            localC = _mm256_sub_epi8(localC, _mm256_cmpeq_epi8(chunk, vC));
            localG = _mm256_sub_epi8(localG, _mm256_cmpeq_epi8(chunk, vG));
            localT = _mm256_sub_epi8(localT, _mm256_cmpeq_epi8(chunk, vT));
            i += 32;
        }

        totalA = _mm256_add_epi64(totalA, _mm256_sad_epu8(localA, zero));
        totalC = _mm256_add_epi64(totalC, _mm256_sad_epu8(localC, zero));
        totalG = _mm256_add_epi64(totalG, _mm256_sad_epu8(localG, zero));
        totalT = _mm256_add_epi64(totalT, _mm256_sad_epu8(localT, zero));
    }

    uint64_t arrA[4], arrC[4], arrG[4], arrT[4];
    _mm256_storeu_si256((__m256i*)arrA, totalA);
    _mm256_storeu_si256((__m256i*)arrC, totalC);
    _mm256_storeu_si256((__m256i*)arrG, totalG);
    _mm256_storeu_si256((__m256i*)arrT, totalT);

    countA += arrA[0] + arrA[1] + arrA[2] + arrA[3];
    countC += arrC[0] + arrC[1] + arrC[2] + arrC[3];
    countG += arrG[0] + arrG[1] + arrG[2] + arrG[3];
    countT += arrT[0] + arrT[1] + arrT[2] + arrT[3];

    for (; i < end; i++) {
        if (data[i] == 'A') countA++;
        else if (data[i] == 'C') countC++;
        else if (data[i] == 'G') countG++;
        else if (data[i] == 'T') countT++;
    }

    counts[0] = countA; counts[1] = countC; counts[2] = countG; counts[3] = countT;
}

void count_simd(const char* data, size_t size, size_t* counts) {
    count_simd_core(data, 0, size, counts);
}

// SIMD + MULTITHREADING

typedef struct {
    const char* data;
    size_t start;
    size_t end;
    size_t local_counts[4];
} ThreadArgsSIMDMT;

void* simd_mt_worker(void* arg) {
    ThreadArgsSIMDMT* args = (ThreadArgsSIMDMT*)arg;
    count_simd_core(args->data, args->start, args->end, args->local_counts);
    return NULL;
}

void count_simd_mt(const char* data, size_t size, size_t* counts) {
    pthread_t threads[NUM_THREADS];
    ThreadArgsSIMDMT args[NUM_THREADS];
    size_t chunk_size = size / NUM_THREADS;

    counts[0] = counts[1] = counts[2] = counts[3] = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].data = data;
        args[i].start = i * chunk_size;
        args[i].end = (i == NUM_THREADS - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, simd_mt_worker, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        for(int j = 0; j < 4; j++) {
            counts[j] += args[i].local_counts[j];
        }
    }
}


int main() {
    printf("Allocating and generating %d MB of DNA data...\n", DNA_SIZE / (1024*1024));
    
    char* dna_data = (char*)_mm_malloc(DNA_SIZE, 32);
    if (!dna_data) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    const char nuc[] = {'A', 'C', 'G', 'T'};
    srand(time(NULL));
    for (size_t i = 0; i < DNA_SIZE; i++) {
        dna_data[i] = nuc[rand() % 4];
    }

    size_t counts_scalar[4], counts_mt[4], counts_simd[4], counts_simd_mt[4];
    double t_start, t_end;

    t_start = get_time();
    count_scalar(dna_data, DNA_SIZE, counts_scalar);
    t_end = get_time();
    double time_scalar = t_end - t_start;

    t_start = get_time();
    count_mt(dna_data, DNA_SIZE, counts_mt);
    t_end = get_time();
    double time_mt = t_end - t_start;

    t_start = get_time();
    count_simd(dna_data, DNA_SIZE, counts_simd);
    t_end = get_time();
    double time_simd = t_end - t_start;

    t_start = get_time();
    count_simd_mt(dna_data, DNA_SIZE, counts_simd_mt);
    t_end = get_time();
    double time_simd_mt = t_end - t_start;

    printf("\nDNA size: %d MB\n", DNA_SIZE / (1024 * 1024));
    printf("Threads used: %d\n", NUM_THREADS);
    printf("Counts (A C G T):\n");
    printf("%zu %zu %zu %zu\n\n", counts_scalar[0], counts_scalar[1], counts_scalar[2], counts_scalar[3]);
    
    printf("Scalar time:                %.3f sec\n", time_scalar);
    printf("Multithreading time:        %.3f sec\n", time_mt);
    printf("SIMD time:                  %.3f sec\n", time_simd);
    printf("SIMD + Multithreading time: %.3f sec\n", time_simd_mt);

    _mm_free(dna_data);
    return 0;
}
