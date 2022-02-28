#ifndef DEADLOCK_H_INCLUDED
#define DEADLOCK_H_INCLUDED

#include <commons/collections/dictionary.h>
#include <stdbool.h>

#include "domain/cola_planificacion.h"
#include "domain/cola_recursos.h"
#include "domain/recurso_sem.h"

typedef struct t_deadlock t_deadlock;

bool deadlock_espera_en_semaforo(t_deadlock*);
pthread_mutex_t* deadlock_get_dict_mutex(t_deadlock*);
t_deadlock* deadlock_create(void);
t_dictionary* deadlock_get_dict(t_deadlock*);
void deadlock_destroy(t_deadlock*);
void deadlock_set_semaforo_en_que_espera(t_deadlock*, t_recurso_sem* sem);
void detectar_y_recuperarse_del_deadlock(t_cola_planificacion* pcbsBlocked, t_cola_planificacion* pcbsSusBlocked, t_cola_recursos* semaforosDelSistema, sem_t* gradoMultiprog);

#endif
