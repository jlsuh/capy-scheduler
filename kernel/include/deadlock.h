#ifndef DEADLOCK_H_INCLUDED
#define DEADLOCK_H_INCLUDED

#include <stdbool.h>

#include "kernel_structs.h"

bool es_este_semaforo(void *recursoSemVoid, void *nombreVoid);
void detectar_y_recuperarse_del_deadlock(t_cola_planificacion *pcbsBlocked, t_cola_planificacion *pcbsSusBlocked, t_cola_recursos *semaforosDelSistema, sem_t *gradoMultiprog);

#endif
