#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define DEFAULT_N 5
#define DEFAULT_MEALS 3

typedef struct {
    int id;
    int meals_to_eat;
} phil_arg_t;

int N = DEFAULT_N;
pthread_mutex_t *forks;    
sem_t butler;              
pthread_t *philos;

static void busy_work(unsigned long iterations) {
    volatile unsigned long x = 0;
    for (unsigned long i = 0; i < iterations; ++i) {
        x += i ^ (i << 1);
    }
    if (x == 0) putchar('\0');
}

void take_forks(int id) {
    sem_wait(&butler);

    int left = id;
    int right = (id + 1) % N;

    pthread_mutex_lock(&forks[left]);
    pthread_mutex_lock(&forks[right]);
}

void put_forks(int id) {
    int left = id;
    int right = (id + 1) % N;

    pthread_mutex_unlock(&forks[right]);
    pthread_mutex_unlock(&forks[left]);

    sem_post(&butler);
}

void *philosopher(void *arg) {
    phil_arg_t *pa = (phil_arg_t *)arg;
    int id = pa->id;
    int meals = pa->meals_to_eat;

    for (int i = 0; i < meals; ++i) {
        printf("[Filósofo %d] pensando (iteração %d/%d)\n", id, i+1, meals);
        busy_work(2000000UL + (id * 100000UL)); 

        printf("[Filósofo %d] com fome e tentando pegar garfos\n", id);
        take_forks(id);

        printf("[Filósofo %d] comendo (iteração %d/%d)\n", id, i+1, meals);
        busy_work(1500000UL + (id * 80000UL));

        put_forks(id);
        printf("[Filósofo %d] terminou de comer e soltou os garfos\n", id);
    }

    printf("[Filósofo %d] terminou todas as refeições e se retira\n", id);
    free(pa);
    return NULL;
}

int main(int argc, char *argv[]) {
    int meals = DEFAULT_MEALS;

    if (argc >= 2) {
        N = atoi(argv[1]);
        if (N <= 1) {
            fprintf(stderr, "N deve ser >= 2. Usando %d.\n", DEFAULT_N);
            N = DEFAULT_N;
        }
    }
    if (argc >= 3) {
        meals = atoi(argv[2]);
        if (meals <= 0) {
            fprintf(stderr, "meals deve ser >= 1. Usando %d.\n", DEFAULT_MEALS);
            meals = DEFAULT_MEALS;
        }
    }

    forks = malloc(sizeof(pthread_mutex_t) * N);
    philos = malloc(sizeof(pthread_t) * N);

    for (int i = 0; i < N; ++i) pthread_mutex_init(&forks[i], NULL);
    sem_init(&butler, 0, N - 1);

    for (int i = 0; i < N; ++i) {
        phil_arg_t *pa = malloc(sizeof(phil_arg_t));
        pa->id = i;
        pa->meals_to_eat = meals;
        if (pthread_create(&philos[i], NULL, philosopher, pa) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < N; ++i) pthread_join(philos[i], NULL);

    for (int i = 0; i < N; ++i) pthread_mutex_destroy(&forks[i]);
    sem_destroy(&butler);

    free(forks);
    free(philos);

    printf("Todos filósofos terminaram.\n");
    return 0;
}
