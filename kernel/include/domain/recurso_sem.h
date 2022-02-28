#ifndef RECURSO_SEM_H_INCLUDED
#define RECURSO_SEM_H_INCLUDED

#include <commons/collections/queue.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct t_recurso_sem t_recurso_sem;

bool es_este_semaforo(void* recursoSemVoid, void* nombreVoid);
bool recurso_sem_es_bloqueante(t_recurso_sem*);
bool recurso_sem_hay_pcbs_bloqueados(t_recurso_sem*);
char* recurso_sem_get_nombre(t_recurso_sem*);
int32_t recurso_sem_get_valor_actual(t_recurso_sem*);
pthread_mutex_t* recurso_sem_get_mutex_cola_pcbs(t_recurso_sem*);
pthread_mutex_t* recurso_sem_get_mutex_valor_sem(t_recurso_sem*);
t_queue* recurso_sem_get_cola_pcbs(t_recurso_sem*);
t_recurso_sem* recurso_sem_create(char* nombre, int32_t valor);
void recurso_sem_dec_cant_instancias(t_recurso_sem*);
void recurso_sem_destroy(t_recurso_sem*);
void recurso_sem_inc_cant_instancias(t_recurso_sem*);

#endif
