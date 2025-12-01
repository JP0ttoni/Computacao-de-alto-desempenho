#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>

#define ALING      64
#define MAX_DIGITS 100000
#define MAX_STEPS  100000000
#define MASK       0x000000000000000F

char *harmonic_progression_sum(uint64_t digit, uint64_t N) {
    char str_digits[] = {'0','1','2','3','4','5','6','7','8','9'};
    uint64_t d_digit = digit + 11;
    uint64_t *digits = NULL;
    assert(posix_memalign((void**)&digits, ALING, sizeof(uint64_t) * d_digit) == 0);
    memset(digits, 0, sizeof(uint64_t) * d_digit);
    int num_threads = omp_get_max_threads();
    uint64_t **local_digits = malloc(sizeof(uint64_t*) * num_threads);
    for (int t = 0; t < num_threads; ++t) {
        assert(posix_memalign((void**)&local_digits[t], ALING, sizeof(uint64_t) * d_digit) == 0);
        memset(local_digits[t], 0, sizeof(uint64_t) * d_digit);
    }
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        uint64_t *local = local_digits[tid];
        #pragma omp for schedule(static)
        for (uint64_t i = 1; i <= N; ++i) {
            uint64_t remainder = 1;
            for (uint64_t i_digit = 0; i_digit < d_digit && remainder; ++i_digit) {
                uint64_t div = remainder / i;
                uint64_t mod = remainder % i;
                local[i_digit] += div;
                remainder = mod * 10;
            }
        }
    }
    for (int t = 0; t < num_threads; ++t) {
        for (uint64_t i = 0; i < d_digit; ++i) {
            digits[i] += local_digits[t][i];
        }
        free(local_digits[t]);
    }
    free(local_digits);
    for (uint64_t i = d_digit - 1; i > 0; --i) {
        digits[i - 1] += digits[i] / 10;
        digits[i] %= 10;
    }
    if (digits[digit + 1] >= 5) ++digits[digit];
    for (uint64_t i = digit; i > 0; --i) {
        digits[i - 1] += digits[i] / 10;
        digits[i] %= 10;
    }
    uint64_t i_part_size = 0, i_part = digits[0], i_mod = 0;
    do {
        i_mod = i_part % 10;
        i_part /= 10;
        if (i_part > 0) i_part_size++;
    } while (i_part != 0);
    char *output = NULL;
    assert(posix_memalign((void**)&output, ALING, (i_part_size + digit + 2)) == 0);
    i_part = digits[0];
    for (uint64_t i_digit = 0; i_digit <= i_part_size; i_digit++) {
        i_mod = i_part % 10;
        i_part /= 10;
        output[i_part_size - i_digit] = str_digits[MASK & i_mod];
    }
    uint64_t offset = i_part_size + 1;
    output[offset++] = ',';
    for (uint64_t i_digit = 1; i_digit <= digit; ++i_digit) {
        output[offset++] = str_digits[MASK & digits[i_digit]];
    }
    output[offset] = '\0';
    free(digits);
    return output;
}

int main(int ac, char **av) {
    if (ac < 3) {
        printf("Uso: %s <digitos> <N>\n", av[0]);
        return EXIT_FAILURE;
    }
    uint64_t digit = strtol(av[1], NULL, 10);
    uint64_t N = strtol(av[2], NULL, 10);
    assert((digit <= MAX_DIGITS) && (N <= MAX_STEPS));
    printf("\nSoma da média harmônica\n");
    printf("\t - Dígitos %lu\n", digit);
    printf("\t - Passos  %lu\n", N);
    char *output = harmonic_progression_sum(digit, N);
    if (output != NULL) {
        printf("\t - Resultado: %s\n", output);
        free(output);
    }
    return EXIT_SUCCESS;
}
