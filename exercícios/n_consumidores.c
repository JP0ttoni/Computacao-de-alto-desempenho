#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define PRODUCE_COUNT 10 
#define SENTINEL -1

typedef struct {
    int buf[BUFFER_SIZE];
    int head; 
    int tail; 
} ring_t;

ring_t ring;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t empty; 
sem_t full;  

int N_CONSUMERS = 3;

void buffer_init() {
    ring.head = ring.tail = 0;
    for (int i = 0; i < BUFFER_SIZE; ++i) ring.buf[i] = 0;
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);
}

void buffer_destroy() {
    sem_destroy(&empty);
    sem_destroy(&full);
}

void buffer_put(int value) {
    sem_wait(&empty);               
    pthread_mutex_lock(&mutex);     
    ring.buf[ring.tail] = value;
    ring.tail = (ring.tail + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&mutex);
    sem_post(&full);               
}

int buffer_get() {
    int value;
    sem_wait(&full);                
    pthread_mutex_lock(&mutex);     
    value = ring.buf[ring.head];
    ring.head = (ring.head + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&mutex);
    sem_post(&empty);               
    return value;
}

void *producer(void *arg) {
    int produce_total = PRODUCE_COUNT;
    for (int i = 1; i <= produce_total; ++i) {
        buffer_put(i);
        printf("[Produtor] produzido: %d\n", i);
    }

    for (int i = 0; i < N_CONSUMERS; ++i) {
        buffer_put(SENTINEL);
    }

    printf("[Produtor] terminou e enviou %d sentinelas\n", N_CONSUMERS);
    return NULL;
}

void *consumer(void *arg) {
    int id = (int)(long)arg;
    while (1) {
        int item = buffer_get();
        if (item == SENTINEL) {
            printf("[Consumidor %d] recebeu SENTINEL e vai encerrar\n", id);
            break;
        }
        printf("[Consumidor %d] consumiu: %d\n", id, item);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        N_CONSUMERS = atoi(argv[1]);
        if (N_CONSUMERS <= 0) {
            fprintf(stderr, "Número de consumidores inválido. Usando 3.\n");
            N_CONSUMERS = 3;
        }
    }

    buffer_init();

    pthread_t prod;
    pthread_t *cons = malloc(sizeof(pthread_t) * N_CONSUMERS);

    if (pthread_create(&prod, NULL, producer, NULL) != 0) {
        perror("pthread_create producer");
        exit(1);
    }

    for (long i = 0; i < N_CONSUMERS; ++i) {
        if (pthread_create(&cons[i], NULL, consumer, (void*)i) != 0) {
            perror("pthread_create consumer");
            exit(1);
        }
    }

    pthread_join(prod, NULL);
    for (int i = 0; i < N_CONSUMERS; ++i) {
        pthread_join(cons[i], NULL);
    }

    buffer_destroy();
    free(cons);
    printf("Todos os consumidores encerraram. Programa finalizado.\n");
    return 0;
}
