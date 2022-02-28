#ifndef COLA_RECURSO_H_INCLUDED
#define COLA_RECURSO_H_INCLUDED

#include <commons/collections/list.h>
#include <pthread.h>

typedef struct t_cola_recursos t_cola_recursos;

t_cola_recursos* cola_recursos_create(void);
t_list* cola_recursos_get_list(t_cola_recursos*);
pthread_mutex_t* cola_recursos_get_mutex(t_cola_recursos*);

#endif
