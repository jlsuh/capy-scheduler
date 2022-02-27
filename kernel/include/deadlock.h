#ifndef DEADLOCK_H_INCLUDED
#define DEADLOCK_H_INCLUDED

#include <stdbool.h>

#include "kernel_structs.h"

typedef struct t_deadlock t_deadlock;

bool es_este_semaforo(void *recursoSemVoid, void *nombreVoid);
void detectar_y_recuperarse_del_deadlock(t_cola_planificacion *pcbsBlocked, t_cola_planificacion *pcbsSusBlocked, t_cola_recursos *semaforosDelSistema, sem_t *gradoMultiprog);
bool deadlock_espera_en_semaforo(t_deadlock *);
t_dictionary *deadlock_get_dict(t_deadlock *);
pthread_mutex_t *deadlock_get_dict_mutex(t_deadlock *);
t_dictionary *deadlock_get_semaforos_que_retiene(t_deadlock *);
void deadlock_set_semaforo_en_que_espera(t_deadlock *, t_recurso_sem *);
t_deadlock *deadlock_create(void);
void deadlock_destroy(t_deadlock *);

#endif
