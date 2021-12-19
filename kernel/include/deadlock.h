#ifndef DEADLOCK_H_INCLUDED
#define DEADLOCK_H_INCLUDED

#include <stdio.h>
#include <string.h>

#include <commons/collections/list.h>
#include <commons/string.h>

#include "common_utils.h"
#include "kernel_structs.h"
#include "scheduler.h"

typedef enum {
    PCB_LIST,
    SEM_LIST
} t_dimention_type;

bool eliminar_celdas_nulas_consecutivas(t_list *firstToIterateList, t_dimention_type firstToIterateType, t_list *pivotList);
bool eliminar_pcb_de_lista(t_pcb *pcb, t_list *lista);
bool es_este_pcb(void *pcbVoid, void *pidVoid);
bool es_este_semaforo(void *recursoSemVoid, void *nombreVoid);
bool espera_al_semaforo(t_pcb *pcb, t_recurso_sem *sem);
bool espera_algun_semaforo(void *pcbVoid);
bool matrices_son_nulas(t_list *pcbsEnDeadlock, t_list *semsEnDeadlock);
bool retiene_instancias_del_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void *mayor_pid(void *pcbVoid1, void *pcbVoid2);
void detectar_y_recuperarse_del_deadlock(t_cola_planificacion *pcbsBlocked, t_cola_planificacion *pcbsSusBlocked, t_cola_recursos *semaforosDelSistema, sem_t *gradoMultiprog);
void finalizar_carpincho_en_deadlock(t_pcb *pcb);
void liberar_instancias_que_retiene(t_pcb *pcb, t_recurso_sem *sem);
void recuperarse_del_deadlock(t_list *pcbsEnDeadlock, t_list *semsEnDeadlock, t_list *listaDeSemaforosDelSistema, t_list *listaDePcbsBlockedDelSistema, t_list *listaDePcbsSusBlockedDelSistema, sem_t *gradoMultiprog);
void reducir_matrices_de_deteccion(t_list *pcbsEnDeadlock, t_list *semsEnDeadlock);

#endif
