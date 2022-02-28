#ifndef RECURSO_IO_H_INCLUDED
#define RECURSO_IO_H_INCLUDED

#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

typedef struct t_recurso_io t_recurso_io;

t_recurso_io* recurso_io_create(char* nombre, uint32_t duracion);
char* recurso_io_get_nombre(t_recurso_io*);
pthread_mutex_t* recurso_io_get_mutex_cola_pcbs(t_recurso_io*);
t_queue* recurso_io_get_cola_pcbs(t_recurso_io*);
sem_t* recurso_io_get_sem_instancias_disponibles(t_recurso_io*);
uint32_t recurso_io_get_duracion(t_recurso_io*);

#endif
