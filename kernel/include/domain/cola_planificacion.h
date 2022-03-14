#ifndef COLA_PLANIFICACION_H_INCLUDED
#define COLA_PLANIFICACION_H_INCLUDED

#include <commons/collections/list.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct t_cola_planificacion t_cola_planificacion;

pthread_mutex_t *cola_planificacion_get_mutex(t_cola_planificacion *);
sem_t *cola_planificacion_get_instancias_disponibles(t_cola_planificacion *);
t_cola_planificacion *cola_planificacion_create(int semInitVal);
t_list *cola_planificacion_get_list(t_cola_planificacion *);
void cola_planificacion_destroy(t_cola_planificacion *);

#endif
