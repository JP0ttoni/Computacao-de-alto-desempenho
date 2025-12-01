#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[]) {

    omp_set_num_threads(12);

    int a_r, a_c, b_r, b_c;
    int i, j, k;
    int nthreads, tid;
    int chunk;

    printf("Informe o numero de linhas da matriz A: ");
    scanf("%d", &a_r);
    printf("Informe o numero de colunas da matriz A: ");
    scanf("%d", &a_c);

    b_r = a_c;

    printf("Informe o numero de colunas da matriz B: ");
    scanf("%d", &b_c);

    chunk = 10;

    int **a = (int**) malloc(a_r * sizeof(int*));
    int **b = (int**) malloc(b_r * sizeof(int*));
    int **c = (int**) malloc(a_r * sizeof(int*));

    for(i = 0; i < a_r; i++){
        a[i] = (int*) malloc(a_c * sizeof(int));
        c[i] = (int*) malloc(b_c * sizeof(int));
    }
    for(i = 0; i < b_r; i++){
        b[i] = (int*) malloc(b_c * sizeof(int));
    }

    double start = omp_get_wtime();

    #pragma omp parallel shared(a,b,c,nthreads,chunk) private(tid,i,j,k)
    {
        tid = omp_get_thread_num();
        if (tid == 0) {
            nthreads = omp_get_num_threads();
            printf("Iniciando multiplicacaao de matrizes com %d threads...\n", nthreads);
        }

        #pragma omp for schedule(static,chunk)
        for(i = 0; i < a_r; i++){
            for(j = 0; j < a_c; j++){
                a[i][j] = 1; 
            }
        }

        #pragma omp for schedule(static,chunk)
        for(i = 0; i < b_r; i++){
            for(j = 0; j < b_c; j++){
                b[i][j] = 1;
            }
        }

        #pragma omp for schedule(static,chunk)
        for(i = 0; i < a_r; i++){
            for(j = 0; j < b_c; j++){
                c[i][j] = 0;
            }
        }

        printf("Thread %d iniciando multiplicacao...\n", tid);

        #pragma omp for schedule(static,chunk)
        for(i = 0; i < a_r; i++){
            for(j = 0; j < b_c; j++){
                for(k = 0; k < a_c; k++){
                    c[i][j] += a[i][k] * b[k][j];
                }
            }
        }
    }

    double end = omp_get_wtime();
    double dif = end - start;

    printf("\nTempo de execucao paralelo: %f segundos\n", dif);

    for(i = 0; i < a_r; i++) free(a[i]);
    for(i = 0; i < b_r; i++) free(b[i]);
    for(i = 0; i < a_r; i++) free(c[i]);
    free(a); free(b); free(c);

    return 0;
}
