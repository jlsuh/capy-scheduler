#ifndef SCHEDULER_H_INCLUDED
#define SCHEDULER_H_INCLUDED

#include "kernel_config.h"
#include "kernel_structs.h"
#include "pcb.h"

bool kernel_sem_wait(t_recurso_sem *sem, t_pcb *pcbWait);
double get_diferencial_de_tiempo(time_t tiempoFinal, time_t tiempoInicial);
double media_exponencial(double realAnterior, double estAnterior);
double response_ratio(t_pcb *pcb, time_t now);
t_cola_recursos *cola_recursos_create(void);
t_pcb *elegir_en_base_a_hrrn(t_cola_planificacion *colaPlanificacion);
t_pcb *elegir_en_base_a_sjf(t_cola_planificacion *colaPlanificacion);
t_pcb *kernel_sem_post(t_recurso_sem *sem, t_pcb *pcbPost);
t_pcb *pcb_create(uint32_t *socket, uint32_t pid);
t_pcb *sjf_pcb_menor_estimacion_entre(t_pcb *unPcb, t_pcb *otroPcb);
t_recurso_sem *recurso_sem_create(char *nombre, int32_t valor);
void *encolar_en_new_nuevo_carpincho_entrante(void *socketHilo);
void pcb_hrrn_est_update(t_pcb *pcb, time_t tiempoFinal, time_t tiempoInicial);
void pcb_hrrn_destroy(t_pcb *pcb);
void inicializar_hrrn(t_pcb *pcb);
void inicializar_sjf(t_pcb *pcb);
void iniciar_planificacion(void);
void liberar_una_instancia_del_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void log_transition(const char *entityName, const char *prev, const char *post, int pid);
void pcb_destroy(t_pcb *pcb);
void recurso_sem_destroy(t_recurso_sem *unSemaforo);
void retener_una_instancia_del_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void pcb_sjf_est_update(t_pcb *pcb, time_t tiempoFinal, time_t tiempoInicial);
void pcb_sjf_destroy(t_pcb *pcb);

#endif
