#include "deadlock.h"

bool espera_al_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    if(espera_algun_semaforo(pcb) && strcmp(pcb->deadlockInfo->esperaEnSemaforo->nombre, sem->nombre) == 0) {
        return true;
    }
    return false;
}

bool retiene_instancias_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    return dictionary_has_key(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre);
}

bool espera_algun_semaforo(void* pcbVoid) {
    t_pcb* pcb = (t_pcb*) pcbVoid;
    return pcb->deadlockInfo->esperaEnSemaforo != NULL;
}

bool es_este_semaforo(void* recursoSemVoid, void* nombreVoid) {
    t_recurso_sem* recursoSem = recursoSemVoid;
    char* nombre = nombreVoid;
    return string_equals_ignore_case(recursoSem->nombre, nombre);
}

bool es_este_pcb(void* pcbVoid, void* pidVoid) {
    t_pcb* pcb = (t_pcb*) pcbVoid;
    uint32_t pid = *(uint32_t*) pidVoid;
    return pcb->pid == pid;
}

void* mayor_pid(void* pcbVoid1, void* pcbVoid2) {
    t_pcb* pcb1 = pcbVoid1;
    t_pcb* pcb2 = pcbVoid2;
    return pcb1->pid > pcb2->pid ? pcb1 : pcb2;
}

bool matrices_son_nulas(t_list* pcbsEnDeadlock, t_list* semsEnDeadlock) {
    return !list_is_empty(pcbsEnDeadlock) && !list_is_empty(semsEnDeadlock);
}

bool eliminar_celdas_nulas_consecutivas(t_list* firstToIterateList, t_dimention_type firstToIterateType, t_list* pivotList) {
    /* Eliminar:
    - Procesos que no poseen retención o espera
    - Recursos que nadie retiene o espera
    */
    int initInnerSize = list_size(firstToIterateList);
    int initPivotSize = list_size(pivotList);
    for(int i = 0; i < list_size(pivotList); i++) {
        t_recurso_sem* sem = NULL;
        t_pcb* pcb = NULL;
        bool espera = false;
        bool retencion = false;
        bool existenCeldasNoNulas = false;
        for(int j = 0; j < list_size(firstToIterateList); j++) {
            /* Fijamos la fila: iteramos todos los semáforos para 1 pcb */
            if(firstToIterateType == PCB_LIST) {
                sem = list_get(pivotList, i);
                pcb = list_get(firstToIterateList, j);
            }
            /* Fijamos la columna: iteramos todos los pcbs para 1 semáforo */
            else if(firstToIterateType == SEM_LIST) {
                sem = list_get(firstToIterateList, j);
                pcb = list_get(pivotList, i);
            }
            /* Armamos las matrices */
            if(!espera) {
                /* Contemplamos la matriz de peticiones pendientes */
                espera = espera_al_semaforo(pcb, sem);
            }
            if(!retencion) {
                /* Contemplamos la matriz de recursos asignados */
                retencion = retiene_instancias_del_semaforo(pcb, sem);
            }
            if(espera && retencion) {
                existenCeldasNoNulas = true;
                break;
            }
        }
        /* Reducción de la lista de la dimensión fijada */
        if(!existenCeldasNoNulas) {
            list_remove(pivotList, i);
            i--;
        }
    }
    return list_size(firstToIterateList) < initInnerSize || list_size(pivotList) < initPivotSize;
}

void reducir_matrices_de_deteccion(t_list* pcbsEnDeadlock, t_list* semsEnDeadlock) {
    if(!list_is_empty(pcbsEnDeadlock) && !list_is_empty(semsEnDeadlock)) {
        bool seEliminaronColumnas = true;
        bool seEliminaronFilas = true;
        while(seEliminaronColumnas || seEliminaronFilas) {
            seEliminaronColumnas = eliminar_celdas_nulas_consecutivas(pcbsEnDeadlock, PCB_LIST, semsEnDeadlock);
            seEliminaronFilas = eliminar_celdas_nulas_consecutivas(semsEnDeadlock, SEM_LIST, pcbsEnDeadlock);
        }
    }
}

bool eliminar_pcb_de_lista(t_pcb* pcb, t_list* lista) {
    int index = list_get_index(lista, (void*) es_este_pcb, (void*) &(pcb->pid));
    if(index != -1) {
        list_remove(lista, index);
        return true;
    }
    return false;
}

void finalizar_carpincho_en_deadlock(t_pcb* pcb) {
    char* prevStatus = NULL;
    if(pcb->status == BLOCKED) {
        prevStatus = string_from_format("BLOCKED");
    } else if(pcb->status == SUSBLOCKED) {
        prevStatus = string_from_format("SUSP/BLOCKED");
    }
    log_info(kernelLogger, "Deadlock: Finalización abrupta del PCB ID %d por resolución de deadlocks", pcb->pid);
    send_empty_buffer(DEADLOCK_END, *(pcb->socket));
    log_transition("Deadlock", prevStatus, "EXIT", pcb->pid);
    pcb->algoritmo_destroy(pcb);
    free(prevStatus);
}

void liberar_instancias_que_retiene(t_pcb* pcb, t_recurso_sem* sem) {
    int cantInstRet = *(int*) dictionary_get(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre);
    while(cantInstRet > 0) {
        kernel_sem_post(sem, pcb);
        cantInstRet--;
    }
}

void recuperarse_del_deadlock(t_list* pcbsEnDeadlock, t_list* semsEnDeadlock, t_list* listaDeSemaforosDelSistema,
                                t_list* listaDePcbsBlockedDelSistema, t_list* listaDePcbsSusBlockedDelSistema, sem_t* gradoMultiprog) {
    t_pcb* pcbDeMayorPID = list_get_maximum(pcbsEnDeadlock, mayor_pid);
    bool yaFueEliminadoDeLasColas = false;
    for(int i = 0; i < list_size(listaDeSemaforosDelSistema); i++) {
        t_recurso_sem* sem = list_get(listaDeSemaforosDelSistema, i);
        if(retiene_instancias_del_semaforo(pcbDeMayorPID, sem)) {
            if(!yaFueEliminadoDeLasColas) {
                bool acabaDeSerEliminado = eliminar_pcb_de_lista(pcbDeMayorPID, listaDePcbsBlockedDelSistema);
                if(!acabaDeSerEliminado) {
                    eliminar_pcb_de_lista(pcbDeMayorPID, listaDePcbsSusBlockedDelSistema);
                } else {
                    sem_post(gradoMultiprog);
                }
                t_recurso_sem* semEnDondeSeEncuentraBloqueado = list_find2(listaDeSemaforosDelSistema, (void*) es_este_semaforo, (void*) (pcbDeMayorPID->deadlockInfo->esperaEnSemaforo->nombre));
                eliminar_pcb_de_lista(pcbDeMayorPID, semEnDondeSeEncuentraBloqueado->colaPCBs->elements);
                yaFueEliminadoDeLasColas = true;
                semEnDondeSeEncuentraBloqueado->valorActual++;
            }
            liberar_instancias_que_retiene(pcbDeMayorPID, sem);
        }
    }
    finalizar_carpincho_en_deadlock(pcbDeMayorPID);
}

void detectar_y_recuperarse_del_deadlock(t_cola_planificacion* pcbsBlocked, t_cola_planificacion* pcbsSusBlocked, t_cola_recursos* semaforosDelSistema, sem_t* gradoMultiprog) {
    t_list* pcbsDeadlockBlocked = list_filter(pcbsBlocked->lista, espera_algun_semaforo);
    t_list* pcbsDeadlockSusBlocked = list_filter(pcbsSusBlocked->lista, espera_algun_semaforo);
    t_list* pcbsEnDeadlock = list_create();
    list_add_all(pcbsEnDeadlock, pcbsDeadlockBlocked);
    list_add_all(pcbsEnDeadlock, pcbsDeadlockSusBlocked);
    if(!list_is_empty(pcbsEnDeadlock)) {
        t_list* semsEnDeadlock = list_create();
        list_add_all(semsEnDeadlock, semaforosDelSistema->listaRecursos);
        bool existeDeadlock = true;
        while(existeDeadlock) {
            reducir_matrices_de_deteccion(pcbsEnDeadlock, semsEnDeadlock);
            existeDeadlock = matrices_son_nulas(pcbsEnDeadlock, semsEnDeadlock);
            if(existeDeadlock) {
                recuperarse_del_deadlock(
                    pcbsEnDeadlock,
                    semsEnDeadlock,
                    semaforosDelSistema->listaRecursos,
                    pcbsBlocked->lista,
                    pcbsSusBlocked->lista,
                    gradoMultiprog
                );
                list_destroy(pcbsDeadlockBlocked);
                list_destroy(pcbsDeadlockSusBlocked);
                list_destroy(pcbsEnDeadlock);
                list_destroy(semsEnDeadlock);
                pcbsDeadlockBlocked = list_filter(pcbsBlocked->lista, espera_algun_semaforo);
                pcbsDeadlockSusBlocked = list_filter(pcbsSusBlocked->lista, espera_algun_semaforo);
                pcbsEnDeadlock = list_create();
                list_add_all(pcbsEnDeadlock, pcbsDeadlockBlocked);
                list_add_all(pcbsEnDeadlock, pcbsDeadlockSusBlocked);
                semsEnDeadlock = list_create();
                list_add_all(semsEnDeadlock, semaforosDelSistema->listaRecursos);
                existeDeadlock = matrices_son_nulas(pcbsEnDeadlock, semsEnDeadlock);
            }
        }
        list_destroy(semsEnDeadlock);
    }
    list_destroy(pcbsEnDeadlock);
    list_destroy(pcbsDeadlockBlocked);
    list_destroy(pcbsDeadlockSusBlocked);
}
