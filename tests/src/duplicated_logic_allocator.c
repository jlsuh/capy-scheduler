#include "duplicated_logic_allocator.h"

double ESTIMACION_INICIAL;
double ALFA;

double get_diferencial_de_tiempo(clock_t tiempoFinal, clock_t tiempoInicial) {
    return (double)(tiempoFinal - tiempoInicial) / CLOCKS_PER_SEC;
}

double media_exponencial(double realAnterior, double estAnterior) {
    // Est(n) = α . R(n-1) + (1 - α) . Est(n-1)
    return ALFA * realAnterior + (1 - ALFA) * estAnterior;
}

double response_ratio(t_pcb* pcb, clock_t now) {
    return get_diferencial_de_tiempo(now, pcb->hrrn->w) / pcb->hrrn->s + 1;
}

t_pcb* elegir_en_base_a_hrrn(t_list* lista) {
    clock_t now = clock();
    t_pcb* pcbConMayorRR = list_get(lista, 0);
    t_pcb* pcbTemp = NULL;

    for (int i = 1; i < list_size(lista); i++) {
        pcbTemp = list_get(lista, i);
        if (response_ratio(pcbTemp, now) > response_ratio(pcbConMayorRR, now)) {
            pcbConMayorRR = pcbTemp;
        }
    }

    return pcbConMayorRR;
}

t_pcb* elegir_en_base_a_sjf(t_list* lista) {
    t_pcb* pcbMenorEstimacion = (t_pcb*)list_get_minimum(lista, (void*)sjf_pcb_menor_estimacion_entre);
    return pcbMenorEstimacion;
}

t_pcb* pcb_create(uint32_t* pid, const char* algoritmo) {
    t_pcb* self = malloc(sizeof(*self));
    self->socket = pid;
    self->status = NEW;
    self->sjf = NULL;
    self->hrrn = NULL;
    if (strcmp(algoritmo, "SJF") == 0) {
        self->algoritmo_init = inicializar_sjf;
        self->algoritmo_destroy = sjf_destroy;
        self->algoritmo_update_next_est_info = sjf_actualizar_info_para_siguiente_estimacion;
    } else if (strcmp(algoritmo, "HRRN") == 0) {
        self->algoritmo_init = inicializar_hrrn;
        self->algoritmo_destroy = hrrn_destroy;
        self->algoritmo_update_next_est_info = hrrn_actualizar_info_para_siguiente_estimacion;
    }
    self->deadlockInfo = malloc(sizeof(*(self->deadlockInfo)));
    self->deadlockInfo->esperaEnSemaforo = NULL;
    self->deadlockInfo->semaforosQueRetiene = dictionary_create();
    pthread_mutex_init(&(self->deadlockInfo->mutexDict), NULL);
    return self;
}

t_pcb* sjf_pcb_menor_estimacion_entre(t_pcb* unPcb, t_pcb* otroPcb) {
    return unPcb->sjf->estActual <= otroPcb->sjf->estActual ? unPcb : otroPcb;
}

void hrrn_actualizar_info_para_siguiente_estimacion(t_pcb* pcb, clock_t tiempoFinal, clock_t tiempoInicial) {
    double realAnterior = get_diferencial_de_tiempo(tiempoFinal, tiempoInicial);
    pcb->hrrn->s = media_exponencial(realAnterior, pcb->hrrn->s);
    pcb->hrrn->w = clock();
}

void hrrn_destroy(t_pcb* pcb) {
    free(pcb->hrrn);
    pcb_destroy(pcb);
}

void inicializar_hrrn(t_pcb* pcb) {
    pcb->hrrn = malloc(sizeof(*(pcb->hrrn)));
    pcb->hrrn->s = media_exponencial(0, ESTIMACION_INICIAL);
    pcb->hrrn->w = clock();
}

void inicializar_sjf(t_pcb* pcb) {
    pcb->sjf = malloc(sizeof(*(pcb->sjf)));
    sjf_actualizar_estimacion_actual(pcb, 0, ESTIMACION_INICIAL);
}

void pcb_destroy(t_pcb* pcb) {
    free(pcb->socket);
    dictionary_destroy_and_destroy_elements(pcb->deadlockInfo->semaforosQueRetiene, free);
    free(pcb->deadlockInfo);
    free(pcb);
}

void sjf_actualizar_estimacion_actual(t_pcb* pcb, double realAnterior, double estAnterior) {
    pcb->sjf->estActual = media_exponencial(realAnterior, estAnterior);
}

void sjf_actualizar_info_para_siguiente_estimacion(t_pcb* pcb, clock_t tiempoFinal, clock_t tiempoInicial) {
    double realAnterior = get_diferencial_de_tiempo(tiempoFinal, tiempoInicial);
    sjf_actualizar_estimacion_actual(pcb, realAnterior, pcb->sjf->estActual);
}

void sjf_destroy(t_pcb* pcb) {
    free(pcb->sjf);
    pcb_destroy(pcb);
}

t_recurso_sem* recurso_sem_create(char* nombre, int32_t valor) {
    t_recurso_sem* recursoSem = malloc(sizeof(*recursoSem));
    recursoSem->colaPCBs = queue_create();
    recursoSem->nombre = strdup(nombre);
    recursoSem->valorInicial = valor;
    recursoSem->valorActual = recursoSem->valorInicial;
    pthread_mutex_init(&(recursoSem->mutexColaPCBs), NULL);
    pthread_mutex_init(&(recursoSem->mutexValorSemaforo), NULL);
    return recursoSem;
}

bool kernel_sem_wait(t_recurso_sem* sem, t_pcb* pcbWait) {
    sem->valorActual--;
    bool esBloqueante = sem->valorActual < 0;
    if (esBloqueante) {
        pcbWait->deadlockInfo->esperaEnSemaforo = sem;
        queue_push(sem->colaPCBs, pcbWait);
    } else {
        retener_una_instancia_del_semaforo(pcbWait, sem);
    }
    return esBloqueante;
}

t_pcb* kernel_sem_post(t_recurso_sem* sem, t_pcb* pcbPost) {
    sem->valorActual++;
    t_pcb* primerPCB = NULL;
    liberar_una_instancia_del_semaforo(pcbPost, sem);
    if (sem->valorActual <= 0) {
        primerPCB = queue_pop(sem->colaPCBs);
        primerPCB->deadlockInfo->esperaEnSemaforo = NULL;
        retener_una_instancia_del_semaforo(primerPCB, sem);
    }
    return primerPCB;
}

void liberar_una_instancia_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    pthread_mutex_lock(&(pcb->deadlockInfo->mutexDict));
    t_dictionary* dict = pcb->deadlockInfo->semaforosQueRetiene;
    int32_t* valorActual = NULL;
    if (dictionary_has_key(dict, sem->nombre)) {
        valorActual = (int32_t*)dictionary_get(dict, sem->nombre);
        (*valorActual)--;
        dictionary_put(dict, sem->nombre, valorActual);
        if (*valorActual == 0) {
            valorActual = dictionary_remove(dict, sem->nombre);
            free(valorActual);
        }
    }
    pthread_mutex_unlock(&(pcb->deadlockInfo->mutexDict));
}

void retener_una_instancia_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    int32_t* valorActual = NULL;
    if (dictionary_has_key(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre)) {
        valorActual = (int32_t*)dictionary_get(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre);
        (*valorActual)++;
    } else {
        valorActual = malloc(sizeof(*valorActual));
        *valorActual = 1;
    }
    dictionary_put(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre, valorActual);
}

void recurso_sem_destroy(t_recurso_sem* unSemaforo) {
    queue_destroy(unSemaforo->colaPCBs);
    pthread_mutex_destroy(&(unSemaforo->mutexColaPCBs));
    pthread_mutex_destroy(&(unSemaforo->mutexValorSemaforo));
    free(unSemaforo->nombre);
    free(unSemaforo);
}

t_cola_recursos* cola_recursos_create(void) {
    t_cola_recursos* colaRecursos = malloc(sizeof(*colaRecursos));
    colaRecursos->listaRecursos = list_create();
    pthread_mutex_init(&(colaRecursos->mutexRecursos), NULL);
    return colaRecursos;
}

void cola_recursos_destroy(t_cola_recursos* colaRecursos) {
    pthread_mutex_destroy(&(colaRecursos->mutexRecursos));
    list_destroy(colaRecursos->listaRecursos);
    free(colaRecursos);
}

bool es_este_pcb(void* pcbVoid, void* pidVoid) {
    t_pcb* pcb = (t_pcb*)pcbVoid;
    uint32_t pid = *(uint32_t*)pidVoid;
    return *(pcb->socket) == pid;
}

void* mayor_pid(void* pcbVoid1, void* pcbVoid2) {
    t_pcb* pcb1 = pcbVoid1;
    t_pcb* pcb2 = pcbVoid2;
    return *(pcb1->socket) > *(pcb2->socket) ? pcb1 : pcb2;
}

bool eliminar_pcb_de_lista(t_pcb* pcb, t_list* lista) {
    int index = list_get_index(lista, (void*)es_este_pcb, (void*)pcb->socket);
    if (index != -1) {
        list_remove(lista, index);
        return true;
    }
    return false;
}
