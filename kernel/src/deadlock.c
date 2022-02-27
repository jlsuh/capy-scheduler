#include "deadlock.h"

#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_utils.h"
#include "kernel_structs.h"
#include "scheduler.h"
#include "stream.h"

extern t_log* kernelLogger;

typedef enum {
    PCB_LIST,
    SEM_LIST
} t_dimention_type;

struct t_deadlock {
    t_dictionary* semaforosQueRetiene;
    t_recurso_sem* esperaEnSemaforo;
    pthread_mutex_t* mutexDict;
};

static bool __deadlock_retiene_instancias_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    return dictionary_has_key(pcb_get_deadlock_info(pcb)->semaforosQueRetiene, sem->nombre);
}

static bool __deadlock_matrices_son_nulas(t_list* pcbsEnDeadlock, t_list* semsEnDeadlock) {
    return !list_is_empty(pcbsEnDeadlock) && !list_is_empty(semsEnDeadlock);
}

static bool __es_este_pcb(void* pcbVoid, void* pidVoid) {
    t_pcb* pcb = (t_pcb*)pcbVoid;
    uint32_t pid = *(uint32_t*)pidVoid;
    return pcb_get_pid(pcb) == pid;
}

static bool __eliminar_celdas_nulas_consecutivas(t_list* firstToIterateList, t_dimention_type firstToIterateType, t_list* pivotList) {
    /* Eliminar:
    - Procesos que no poseen retención o espera
    - Recursos que nadie retiene o espera
    */
    int initInnerSize = list_size(firstToIterateList);
    int initPivotSize = list_size(pivotList);
    for (int i = 0; i < list_size(pivotList); i++) {
        t_recurso_sem* sem = NULL;
        t_pcb* pcb = NULL;
        bool espera = false;
        bool retencion = false;
        bool existenCeldasNoNulas = false;
        for (int j = 0; j < list_size(firstToIterateList); j++) {
            /* Fijamos la fila: iteramos todos los semáforos para 1 pcb */
            if (firstToIterateType == PCB_LIST) {
                sem = list_get(pivotList, i);
                pcb = list_get(firstToIterateList, j);
            } else if (firstToIterateType == SEM_LIST) {
                sem = list_get(firstToIterateList, j);
                pcb = list_get(pivotList, i);
            }
            if (!espera) {
                /* Contemplamos la matriz de peticiones pendientes */
                if (pcb_espera_algun_semaforo(pcb) && strcmp(pcb_get_deadlock_info(pcb)->esperaEnSemaforo->nombre, sem->nombre) == 0) {
                    espera = true;
                }
            }
            if (!retencion) {
                /* Contemplamos la matriz de recursos asignados */
                retencion = __deadlock_retiene_instancias_del_semaforo(pcb, sem);
            }
            if (espera && retencion) {
                existenCeldasNoNulas = true;
                break;
            }
        }
        /* Reducción de la lista de la dimensión fijada */
        if (!existenCeldasNoNulas) {
            list_remove(pivotList, i);
            i--;
        }
    }
    return list_size(firstToIterateList) < initInnerSize || list_size(pivotList) < initPivotSize;
}

static void __reducir_matrices_de_deteccion(t_list* pcbsEnDeadlock, t_list* semsEnDeadlock) {
    if (!list_is_empty(pcbsEnDeadlock) && !list_is_empty(semsEnDeadlock)) {
        bool seEliminaronColumnas = true;
        bool seEliminaronFilas = true;
        while (seEliminaronColumnas || seEliminaronFilas) {
            seEliminaronColumnas = __eliminar_celdas_nulas_consecutivas(pcbsEnDeadlock, PCB_LIST, semsEnDeadlock);
            seEliminaronFilas = __eliminar_celdas_nulas_consecutivas(semsEnDeadlock, SEM_LIST, pcbsEnDeadlock);
        }
    }
}

static bool __eliminar_pcb_de_lista(t_pcb* pcb, t_list* lista) {
    uint32_t pid = pcb_get_pid(pcb);
    int index = list_get_index(lista, (void*)__es_este_pcb, (void*)&pid);
    if (index != -1) {
        list_remove(lista, index);
        return true;
    }
    return false;
}

static void __finalizar_carpincho_en_deadlock(t_pcb* pcb) {
    char* prevStatus = NULL;
    if (pcb_get_status(pcb) == BLOCKED) {
        prevStatus = string_from_format("BLOCKED");
    } else if (pcb_get_status(pcb) == SUSBLOCKED) {
        prevStatus = string_from_format("SUSP/BLOCKED");
    }
    log_info(kernelLogger, "Deadlock: Finalización abrupta del PCB ID %d por resolución de deadlocks", pcb_get_pid(pcb));
    send_empty_buffer(DEADLOCK_END, *pcb_get_socket(pcb));
    log_transition("Deadlock", prevStatus, "EXIT", pcb_get_pid(pcb));
    pcb_algoritmo_destroy(pcb);
    free(prevStatus);
}

static void __liberar_instancias_que_retiene(t_pcb* pcb, t_recurso_sem* sem) {
    int cantInstRet = *(int*)dictionary_get(pcb_get_deadlock_info(pcb)->semaforosQueRetiene, sem->nombre);
    while (cantInstRet > 0) {
        kernel_sem_post(sem, pcb);
        cantInstRet--;
    }
}

static void __recuperarse_del_deadlock(t_list* pcbsEnDeadlock, t_list* semsEnDeadlock, t_list* listaDeSemaforosDelSistema,
                                       t_list* listaDePcbsBlockedDelSistema, t_list* listaDePcbsSusBlockedDelSistema, sem_t* gradoMultiprog) {
    t_pcb* pcbDeMayorPID = list_get_maximum(pcbsEnDeadlock, pcb_maximum_pid);
    bool yaFueEliminadoDeLasColas = false;
    for (int i = 0; i < list_size(listaDeSemaforosDelSistema); i++) {
        t_recurso_sem* sem = list_get(listaDeSemaforosDelSistema, i);
        if (__deadlock_retiene_instancias_del_semaforo(pcbDeMayorPID, sem)) {
            if (!yaFueEliminadoDeLasColas) {
                bool acabaDeSerEliminado = __eliminar_pcb_de_lista(pcbDeMayorPID, listaDePcbsBlockedDelSistema);
                if (!acabaDeSerEliminado) {
                    __eliminar_pcb_de_lista(pcbDeMayorPID, listaDePcbsSusBlockedDelSistema);
                } else {
                    sem_post(gradoMultiprog);
                }
                t_recurso_sem* semEnDondeSeEncuentraBloqueado = list_find2(listaDeSemaforosDelSistema, (void*)es_este_semaforo, (void*)(pcb_get_deadlock_info(pcbDeMayorPID)->esperaEnSemaforo->nombre));
                __eliminar_pcb_de_lista(pcbDeMayorPID, semEnDondeSeEncuentraBloqueado->colaPCBs->elements);
                yaFueEliminadoDeLasColas = true;
                semEnDondeSeEncuentraBloqueado->valorActual++;
            }
            __liberar_instancias_que_retiene(pcbDeMayorPID, sem);
        }
    }
    __finalizar_carpincho_en_deadlock(pcbDeMayorPID);
}

t_deadlock* deadlock_create(void) {
    t_deadlock* self = malloc(sizeof(*self));
    self->semaforosQueRetiene = dictionary_create();
    self->esperaEnSemaforo = NULL;
    self->mutexDict = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(self->mutexDict, NULL);
    return self;
}

void deadlock_destroy(t_deadlock* self) {
    t_dictionary* dict = deadlock_get_semaforos_que_retiene(self);
    dictionary_destroy_and_destroy_elements(dict, free);
    pthread_mutex_destroy(deadlock_get_dict_mutex(self));
    free(deadlock_get_dict_mutex(self));
    free(self);
}

bool deadlock_espera_en_semaforo(t_deadlock* self) {
    return self->esperaEnSemaforo != NULL;
}

t_dictionary* deadlock_get_dict(t_deadlock* self) {
    return self->semaforosQueRetiene;
}

pthread_mutex_t* deadlock_get_dict_mutex(t_deadlock* self) {
    return self->mutexDict;
}

t_dictionary* deadlock_get_semaforos_que_retiene(t_deadlock* self) {
    return self->semaforosQueRetiene;
}

void deadlock_set_semaforo_en_que_espera(t_deadlock* self, t_recurso_sem* sem) {
    self->esperaEnSemaforo = sem;
}

// TODO
bool es_este_semaforo(void* recursoSemVoid, void* nombreVoid) {
    t_recurso_sem* recursoSem = recursoSemVoid;
    char* nombre = nombreVoid;
    return string_equals_ignore_case(recursoSem->nombre, nombre);
}

void detectar_y_recuperarse_del_deadlock(t_cola_planificacion* pcbsBlocked, t_cola_planificacion* pcbsSusBlocked, t_cola_recursos* semaforosDelSistema, sem_t* gradoMultiprog) {
    t_list* pcbsDeadlockBlocked = list_filter(pcbsBlocked->lista, pcb_espera_algun_semaforo);
    t_list* pcbsDeadlockSusBlocked = list_filter(pcbsSusBlocked->lista, pcb_espera_algun_semaforo);
    t_list* pcbsEnDeadlock = list_create();
    list_add_all(pcbsEnDeadlock, pcbsDeadlockBlocked);
    list_add_all(pcbsEnDeadlock, pcbsDeadlockSusBlocked);
    if (!list_is_empty(pcbsEnDeadlock)) {
        t_list* semsEnDeadlock = list_create();
        list_add_all(semsEnDeadlock, semaforosDelSistema->listaRecursos);
        bool existeDeadlock = true;
        while (existeDeadlock) {
            __reducir_matrices_de_deteccion(pcbsEnDeadlock, semsEnDeadlock);
            existeDeadlock = __deadlock_matrices_son_nulas(pcbsEnDeadlock, semsEnDeadlock);
            if (existeDeadlock) {
                __recuperarse_del_deadlock(
                    pcbsEnDeadlock,
                    semsEnDeadlock,
                    semaforosDelSistema->listaRecursos,
                    pcbsBlocked->lista,
                    pcbsSusBlocked->lista,
                    gradoMultiprog);
                list_destroy(pcbsDeadlockBlocked);
                list_destroy(pcbsDeadlockSusBlocked);
                list_destroy(pcbsEnDeadlock);
                list_destroy(semsEnDeadlock);
                pcbsDeadlockBlocked = list_filter(pcbsBlocked->lista, pcb_espera_algun_semaforo);
                pcbsDeadlockSusBlocked = list_filter(pcbsSusBlocked->lista, pcb_espera_algun_semaforo);
                pcbsEnDeadlock = list_create();
                list_add_all(pcbsEnDeadlock, pcbsDeadlockBlocked);
                list_add_all(pcbsEnDeadlock, pcbsDeadlockSusBlocked);
                semsEnDeadlock = list_create();
                list_add_all(semsEnDeadlock, semaforosDelSistema->listaRecursos);
                existeDeadlock = __deadlock_matrices_son_nulas(pcbsEnDeadlock, semsEnDeadlock);
            }
        }
        list_destroy(semsEnDeadlock);
    }
    list_destroy(pcbsEnDeadlock);
    list_destroy(pcbsDeadlockBlocked);
    list_destroy(pcbsDeadlockSusBlocked);
}
