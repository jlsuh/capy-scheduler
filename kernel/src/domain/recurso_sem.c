#include "domain/recurso_sem.h"

#include <commons/collections/queue.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct t_recurso_sem {
    t_queue* colaPCBs;
    char* nombre;
    int32_t valorInicial;
    int32_t valorActual;
    pthread_mutex_t* mutexColaPCBs;
    pthread_mutex_t* mutexValorSemaforo;
};

t_recurso_sem* recurso_sem_create(char* nombre, int32_t valor) {
    t_recurso_sem* self = malloc(sizeof(*self));
    self->colaPCBs = queue_create();
    self->nombre = strdup(nombre);
    self->valorInicial = valor;
    self->valorActual = self->valorInicial;
    self->mutexColaPCBs = malloc(sizeof(*(self->mutexColaPCBs)));
    self->mutexValorSemaforo = malloc(sizeof(*(self->mutexValorSemaforo)));
    pthread_mutex_init(self->mutexColaPCBs, NULL);
    pthread_mutex_init(self->mutexValorSemaforo, NULL);
    return self;
}

void recurso_sem_destroy(t_recurso_sem* unSemaforo) {
    queue_destroy(unSemaforo->colaPCBs);
    pthread_mutex_destroy(unSemaforo->mutexColaPCBs);
    pthread_mutex_destroy(unSemaforo->mutexValorSemaforo);
    free(unSemaforo->mutexColaPCBs);
    free(unSemaforo->mutexValorSemaforo);
    free(unSemaforo->nombre);
    free(unSemaforo);
}

void recurso_sem_inc_cant_instancias(t_recurso_sem* self) {
    self->valorActual++;
}

void recurso_sem_dec_cant_instancias(t_recurso_sem* self) {
    self->valorActual--;
}

bool recurso_sem_es_bloqueante(t_recurso_sem* self) {
    return self->valorActual < 0;
}

bool recurso_sem_hay_pcbs_bloqueados(t_recurso_sem* self) {
    return self->valorActual <= 0;
}

bool es_este_semaforo(void* recursoSemVoid, void* nombreVoid) {
    t_recurso_sem* self = recursoSemVoid;
    char* nombre = nombreVoid;
    return string_equals_ignore_case(self->nombre, nombre);
}

t_queue* recurso_sem_get_cola_pcbs(t_recurso_sem* self) {
    return self->colaPCBs;
}

char* recurso_sem_get_nombre(t_recurso_sem* self) {
    return self->nombre;
}

int32_t recurso_sem_get_valor_actual(t_recurso_sem* self) {
    return self->valorActual;
}

pthread_mutex_t* recurso_sem_get_mutex_cola_pcbs(t_recurso_sem* self) {
    return self->mutexColaPCBs;
}

pthread_mutex_t* recurso_sem_get_mutex_valor_sem(t_recurso_sem* self) {
    return self->mutexValorSemaforo;
}
