#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <immintrin.h>
#include <string.h>
#include <ctype.h>

#define NUM_THREADS 4
#define W_R 77
#define W_G 150
#define W_B 29

typedef struct {
    const uint8_t* in_data;
    uint8_t* out_data;
    size_t start_pixel;
    size_t end_pixel;
} ThreadArgs;

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void skip_comments(FILE *fp) {
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (isspace(ch)) {
            continue;
        } else if (ch == '#') {
            while ((ch = fgetc(fp)) != '\n' && ch != EOF);
        } else {
            ungetc(ch, fp);
            break;
        }
    }
}

uint8_t* read_ppm(const char* filename, int* width, int* height) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Error: Cannot open %s\n", filename);
        return NULL;
    }

    char magic[3];
    if (fscanf(fp, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        printf("Error: Unsupported format. Only P6 PPM is supported.\n");
        fclose(fp);
        return NULL;
    }

    skip_comments(fp);
    if (fscanf(fp, "%d", width) != 1) return NULL;
    skip_comments(fp);
    if (fscanf(fp, "%d", height) != 1) return NULL;
    skip_comments(fp);
    
    int max_val;
    if (fscanf(fp, "%d", &max_val) != 1) return NULL;
    
    fgetc(fp);

    size_t total_bytes = (*width) * (*height) * 3;
    uint8_t* data = (uint8_t*)_mm_malloc(total_bytes, 32);
    
    if (fread(data, 1, total_bytes, fp) != total_bytes) {
        printf("Warning: File size does not match header dimensions.\n");
    }

    fclose(fp);
    return data;
}

void write_ppm(const char* filename, const uint8_t* data, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf("Error: Cannot write to %s\n", filename);
        return;
    }
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    fwrite(data, 1, (size_t)width * height * 3, fp);
    fclose(fp);
}

// SCALAR

void process_scalar(const uint8_t* in, uint8_t* out, size_t start_pixel, size_t end_pixel) {
    for (size_t i = start_pixel; i < end_pixel; i++) {
        size_t idx = i * 3;
        uint32_t gray = (in[idx] * W_R + in[idx+1] * W_G + in[idx+2] * W_B) >> 8;
        out[idx] = out[idx + 1] = out[idx + 2] = (uint8_t)gray;
    }
}

// MULTITHREADING

void* mt_worker_scalar(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    process_scalar(args->in_data, args->out_data, args->start_pixel, args->end_pixel);
    return NULL;
}

void run_multithreaded(const uint8_t* in, uint8_t* out, size_t total_pixels) {
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    size_t chunk_size = total_pixels / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].in_data = in;
        args[i].out_data = out;
        args[i].start_pixel = i * chunk_size;
        args[i].end_pixel = (i == NUM_THREADS - 1) ? total_pixels : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, mt_worker_scalar, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

// SIMD

void process_simd(const uint8_t* in, uint8_t* out, size_t start_pixel, size_t end_pixel) {
    size_t i = start_pixel;
    __m256i v_wr = _mm256_set1_epi32(W_R);
    __m256i v_wg = _mm256_set1_epi32(W_G);
    __m256i v_wb = _mm256_set1_epi32(W_B);

    for (; i + 7 < end_pixel; i += 8) {
        size_t idx = i * 3;
        __m256i r = _mm256_set_epi32(in[idx+21], in[idx+18], in[idx+15], in[idx+12], in[idx+9], in[idx+6], in[idx+3], in[idx+0]);
        __m256i g = _mm256_set_epi32(in[idx+22], in[idx+19], in[idx+16], in[idx+13], in[idx+10], in[idx+7], in[idx+4], in[idx+1]);
        __m256i b = _mm256_set_epi32(in[idx+23], in[idx+20], in[idx+17], in[idx+14], in[idx+11], in[idx+8], in[idx+5], in[idx+2]);

        __m256i sum_r = _mm256_mullo_epi32(r, v_wr);
        __m256i sum_g = _mm256_mullo_epi32(g, v_wg);
        __m256i sum_b = _mm256_mullo_epi32(b, v_wb);
        __m256i total = _mm256_add_epi32(sum_r, _mm256_add_epi32(sum_g, sum_b));
        __m256i gray = _mm256_srli_epi32(total, 8);

        uint32_t gray_arr[8] __attribute__((aligned(32)));
        _mm256_store_si256((__m256i*)gray_arr, gray);

        for (int p = 0; p < 8; p++) {
            uint8_t g_val = (uint8_t)gray_arr[p];
            out[idx + p*3 + 0] = g_val;
            out[idx + p*3 + 1] = g_val;
            out[idx + p*3 + 2] = g_val;
        }
    }
    process_scalar(in, out, i, end_pixel);
}

// SIMD + MULTITHREADING

void* mt_worker_simd(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    process_simd(args->in_data, args->out_data, args->start_pixel, args->end_pixel);
    return NULL;
}

void run_simd_multithreaded(const uint8_t* in, uint8_t* out, size_t total_pixels) {
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    size_t chunk_size = total_pixels / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].in_data = in;
        args[i].out_data = out;
        args[i].start_pixel = i * chunk_size;
        args[i].end_pixel = (i == NUM_THREADS - 1) ? total_pixels : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, mt_worker_simd, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}


int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <input_image.ppm>\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    int width, height;
    
    printf("Loading %s...\n", input_file);
    uint8_t* img_in = read_ppm(input_file, &width, &height);
    if (!img_in) return 1;

    size_t total_pixels = (size_t)width * height;
    size_t total_bytes = total_pixels * 3;

    uint8_t* img_out_scalar  = (uint8_t*)_mm_malloc(total_bytes, 32);
    uint8_t* img_out_mt      = (uint8_t*)_mm_malloc(total_bytes, 32);
    uint8_t* img_out_simd    = (uint8_t*)_mm_malloc(total_bytes, 32);
    uint8_t* img_out_simd_mt = (uint8_t*)_mm_malloc(total_bytes, 32);

    double t_start, t_end;

    t_start = get_time();
    process_scalar(img_in, img_out_scalar, 0, total_pixels);
    t_end = get_time();
    double time_scalar = t_end - t_start;

    t_start = get_time();
    process_simd(img_in, img_out_simd, 0, total_pixels);
    t_end = get_time();
    double time_simd = t_end - t_start;

    t_start = get_time();
    run_multithreaded(img_in, img_out_mt, total_pixels);
    t_end = get_time();
    double time_mt = t_end - t_start;

    t_start = get_time();
    run_simd_multithreaded(img_in, img_out_simd_mt, total_pixels);
    t_end = get_time();
    double time_simd_mt = t_end - t_start;

    int passed = 1;
    for (size_t i = 0; i < total_bytes; i++) {
        if (img_out_scalar[i] != img_out_simd_mt[i]) {
            passed = 0;
            break;
        }
    }

    const char* output_file = "gray_output.ppm";
    write_ppm(output_file, img_out_simd_mt, width, height);

    printf("\nImage size: %d x %d\n", width, height);
    printf("Threads used: %d\n\n", NUM_THREADS);
    
    printf("Scalar time:                %.3f sec\n", time_scalar);
    printf("SIMD time:                  %.3f sec\n", time_simd);
    printf("Multithreading time:        %.3f sec\n", time_mt);
    printf("Multithreading + SIMD time: %.3f sec\n\n", time_simd_mt);

    printf("Verification: %s\n", passed ? "PASSED" : "FAILED");
    printf("Output image: %s\n", output_file);

    _mm_free(img_in);
    _mm_free(img_out_scalar);
    _mm_free(img_out_mt);
    _mm_free(img_out_simd);
    _mm_free(img_out_simd_mt);

    return 0;
}