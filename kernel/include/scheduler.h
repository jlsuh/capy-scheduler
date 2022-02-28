#ifndef SCHEDULER_H_INCLUDED
#define SCHEDULER_H_INCLUDED

#include "domain/cola_recursos.h"
#include "domain/pcb.h"

bool kernel_sem_wait(t_recurso_sem*, t_pcb*);
t_pcb* elegir_en_base_a_hrrn(t_cola_planificacion*);
t_pcb* elegir_en_base_a_sjf(t_cola_planificacion*);
t_pcb* kernel_sem_post(t_recurso_sem*, t_pcb*);
void* encolar_en_new_nuevo_carpincho_entrante(void* socketHilo);
void iniciar_planificacion(void);
void liberar_una_instancia_del_semaforo(t_pcb*, t_recurso_sem*);
void log_transition(const char* entityName, const char* prev, const char* post, int pid);
void retener_una_instancia_del_semaforo(t_pcb*, t_recurso_sem*);

#endif
