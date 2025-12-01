/*
 * matmul_blocked.c — versão compatível com MinGW antigo (GCC 6.x)
 *
 * Ajustes:
 *  - aligned_alloc substituído por _aligned_malloc (Windows)
 *  - free substituído por _aligned_free
 *  - inclusão de <malloc.h>
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>
#include <malloc.h> 

#ifdef _WIN32
    #define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
    #define aligned_free(ptr) _aligned_free(ptr)
#else
    #define aligned_free(ptr) free(ptr)
#endif

static inline size_t min_size(size_t a, size_t b) { return (a < b) ? a : b; }

double *alloc_matrix(int N) {
    size_t bytes = (size_t)N * N * sizeof(double);
    double *m = (double*) aligned_alloc(64, bytes);
    if (!m) {
        fprintf(stderr, "Falha ao alocar matriz %d x %d\n", N, N);
        exit(1);
    }
    return m;
}

void init_matrix(double *M, int N, double val) {
    size_t total = (size_t)N * N;
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < total; ++i)
        M[i] = val;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Uso: %s N threads [block_size] [l3_bytes]\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int nthreads = atoi(argv[2]);
    int B_user = 0;
    long long l3_bytes = 0;
    if (argc >= 4) B_user = atoi(argv[3]);
    if (argc >= 5) l3_bytes = atoll(argv[4]);

    omp_set_num_threads(nthreads);

    const size_t elem_size = sizeof(double);

    int B;
    if (B_user > 0) {
        B = B_user;
    } else {
        if (l3_bytes <= 0) {
            B = 64;
        } else {
            double approx = sqrt((double)l3_bytes / (3.0 * (double)elem_size));
            B = (int)approx;
            if (B < 16) B = 16;
            B = (B / 8) * 8;
            if (B == 0) B = 16;
        }
    }

    printf("N=%d threads=%d block_size=%d l3_bytes=%lld\n", N, nthreads, B, l3_bytes);

    double *A = alloc_matrix(N);
    double *Bmat = alloc_matrix(N);
    double *C = alloc_matrix(N);

    init_matrix(A, N, 1.0);
    init_matrix(Bmat, N, 1.0);
    init_matrix(C, N, 0.0);

    double t_start = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp for collapse(2) schedule(dynamic)
        for (int i0 = 0; i0 < N; i0 += B) {
            for (int j0 = 0; j0 < N; j0 += B) {
                int i1 = (int)min_size(N, i0 + B);
                int j1 = (int)min_size(N, j0 + B);

                for (int k0 = 0; k0 < N; k0 += B) {
                    int k1 = (int)min_size(N, k0 + B);

                    for (int i = i0; i < i1; ++i) {
                        for (int k = k0; k < k1; ++k) {
                            double a_val = A[(size_t)i * N + k];
                            #pragma omp simd
                            for (int j = j0; j < j1; ++j) {
                                C[(size_t)i * N + j] += a_val * Bmat[(size_t)k * N + j];
                            }
                        }
                    }
                }
            }
        }
    }

    double t_end = omp_get_wtime();
    double elapsed = t_end - t_start;

    printf("Tempo (com bloqueamento): %f s\n", elapsed);

    int check_ok = 1;
    for (int ii = 0; ii < N; ii += (N/8 > 0 ? N/8 : 1)) {
        int jj = ii;
        double expected = (double)N;
        double diff = fabs(C[(size_t)ii * N + jj] - expected);
        if (diff > 1e-6) { check_ok = 0; break; }
    }

    printf("Checagem amostral: %s\n", check_ok ? "OK" : "FALHOU");

    aligned_free(A);
    aligned_free(Bmat);
    aligned_free(C);

    return 0;
}
