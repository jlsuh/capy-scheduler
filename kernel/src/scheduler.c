#include "scheduler.h"

#include <commons/string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#include "buffer.h"
#include "common_utils.h"
#include "deadlock.h"
#include "mem_adapter.h"
#include "recurso_io.h"
#include "stream.h"

extern t_log* kernelLogger;
extern t_kernel_config* kernelCfg;

static uint32_t nextPid;

static sem_t gradoMultiprog;
static sem_t despertarMedianoPlazo;
static sem_t suspensionConcluida;
static sem_t hayPCBsParaAgregarAlSistema;
static sem_t transicionarSusReadyAready;

pthread_mutex_t mutexMemSocket;
static pthread_mutex_t mutexDeadlock;
static pthread_mutex_t mutexPid;

static t_cola_planificacion* pcbsNew;
static t_cola_planificacion* pcbsReady;
static t_cola_planificacion* pcbsBlocked;
static t_cola_planificacion* pcbsSusReady;
static t_cola_planificacion* pcbsSusBlocked;
static t_cola_planificacion* pcbsExit;
static t_cola_planificacion* pcbsExec;

static t_cola_recursos* semaforosDelSistema;
static t_cola_recursos* dispositivosIODelSistema;

static int get_pcb_index(t_pcb* pcb, t_list* lista) {
    for (int posicion = 0; posicion < list_size(lista); posicion++) {
        if (pcb == (t_pcb*)list_get(lista, posicion)) {
            return posicion;
        }
    }
    return -1;
}

static void enqueue_pcb(t_pcb* pcb, t_cola_planificacion* cola) {
    pthread_mutex_t* mutex = cola_planificacion_get_mutex(cola);
    pthread_mutex_lock(mutex);
    list_add(cola_planificacion_get_list(cola), pcb);
    pthread_mutex_unlock(mutex);
}

static void dequeue_pcb(t_pcb* pcb, t_cola_planificacion* cola) {
    pthread_mutex_t* mutex = cola_planificacion_get_mutex(cola);
    pthread_mutex_lock(mutex);
    int posicion = get_pcb_index(pcb, cola_planificacion_get_list(cola));
    if (posicion != -1) {
        list_remove(cola_planificacion_get_list(cola), posicion);
    } else {
        log_error(kernelLogger, "Kernel: No existe tal elemento en la cola");
    }
    pthread_mutex_unlock(mutex);
}

static void pasar_de_susblocked_a_susready(t_pcb* pcb) {
    dequeue_pcb(pcb, pcbsSusBlocked);
    pcb_set_status(pcb, SUSREADY);
    enqueue_pcb(pcb, pcbsSusReady);
    sem_post(&hayPCBsParaAgregarAlSistema);
}

static void pasar_de_blocked_a_ready(t_pcb* pcb) {
    dequeue_pcb(pcb, pcbsBlocked);
    pcb_set_status(pcb, READY);
    enqueue_pcb(pcb, pcbsReady);
    sem_post(cola_planificacion_get_instancias_disponibles(pcbsReady));
}

static void inicializar_dispositivos_io(t_cola_recursos* dispositivosIODelSistema) {
    uint32_t duracionDispositivo;
    t_recurso_io* recursoIO;
    char** duracionesIO = kernel_config_get_duracionesIO(kernelCfg);
    char** dispositivosIO = kernel_config_get_dispositivosIO(kernelCfg);
    for (int i = 0; i < string_array_size(duracionesIO); i++) {
        duracionDispositivo = atoi(duracionesIO[i]);
        recursoIO = recurso_io_create(dispositivosIO[i], duracionDispositivo);
        list_add(cola_recursos_get_list(dispositivosIODelSistema), recursoIO);
        log_info(kernelLogger, "Kernel: Inicialización de recurso %s con duración de I/O de %d milisegundos", recurso_io_get_nombre(recursoIO), recurso_io_get_duracion(recursoIO));
    }
}

static void ejecutar_rafagas_io(t_recurso_io* recursoIO, t_pcb* primerPCB) {
    log_info(kernelLogger, "Dispositivo I/O <%s>: Carpincho ID %d ejecutando ráfagas I/O", recurso_io_get_nombre(recursoIO), pcb_get_pid(primerPCB));
    intervalo_de_pausa(recurso_io_get_duracion(recursoIO));
    log_info(kernelLogger, "Dispositivo I/O <%s>: Fin ráfagas I/O Carpincho ID %d", recurso_io_get_nombre(recursoIO), pcb_get_pid(primerPCB));
}

static noreturn void* iniciar_rutina_ejecucion_dispositivos_io(void* args) {
    t_recurso_io* recursoIO = (t_recurso_io*)args;
    for (;;) {
        char* nombreDispositivo = string_from_format("Dispositivo I/O <%s>", recurso_io_get_nombre(recursoIO));

        sem_wait(recurso_io_get_sem_instancias_disponibles(recursoIO));

        pthread_mutex_t* mutex = recurso_io_get_mutex_cola_pcbs(recursoIO);
        pthread_mutex_lock(mutex);
        t_pcb* primerPCB = queue_pop(recurso_io_get_cola_pcbs(recursoIO));
        pthread_mutex_unlock(mutex);

        ejecutar_rafagas_io(recursoIO, primerPCB);

        t_status pcbCurrStatus = pcb_get_status(primerPCB);
        if (pcbCurrStatus == SUSBLOCKED) {
            pthread_mutex_lock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
            pasar_de_susblocked_a_susready(primerPCB);
            pthread_mutex_unlock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
            log_transition(nombreDispositivo, "SUSP/BLOCKED", "SUSP/READY", pcb_get_pid(primerPCB));
        } else if (pcbCurrStatus == BLOCKED) {
            pthread_mutex_lock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
            pasar_de_blocked_a_ready(primerPCB);
            pthread_mutex_unlock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
            log_transition(nombreDispositivo, "BLOCKED", "READY", pcb_get_pid(primerPCB));
        }
        free(nombreDispositivo);
    }
}

static void iniciar_dispositivo_io(t_recurso_io* recursoIO) {
    pthread_t th;
    pthread_create(&th, NULL, iniciar_rutina_ejecucion_dispositivos_io, recursoIO);
    pthread_detach(th);
}

static void iniciar_ejecucion_lista_dispositivos_io(void) {
    list_iterate(cola_recursos_get_list(dispositivosIODelSistema), (void*)iniciar_dispositivo_io);
}

static t_pcb* get_and_remove_first_pcb_from_queue(t_cola_planificacion* cola) {
    t_pcb* pcb = NULL;
    pthread_mutex_t* mutex = cola_planificacion_get_mutex(cola);
    pthread_mutex_lock(mutex);
    t_list* listaPCBs = cola_planificacion_get_list(cola);
    if (!list_is_empty(listaPCBs)) {
        pcb = (t_pcb*)list_remove(listaPCBs, 0);
    }
    pthread_mutex_unlock(mutex);
    return pcb;
}

static noreturn void* liberar_carpinchos_en_exit(void* _) {
    log_info(kernelLogger, "Largo Plazo: Hilo liberador de PCBs de Carpinchos en EXIT inicializado");
    for (;;) {
        sem_t* instanciasDisponibles = cola_planificacion_get_instancias_disponibles(pcbsExit);
        sem_wait(instanciasDisponibles);

        t_pcb* pcbALiberar = get_and_remove_first_pcb_from_queue(pcbsExit);

        enviar_mate_close_a_memoria(pcbALiberar);

        send_empty_buffer(OK_FINISH, *pcb_get_socket(pcbALiberar));
        log_info(kernelLogger, "Kernel: Desconexión Carpincho ID %d", pcb_get_pid(pcbALiberar));
        pcb_algoritmo_destroy(pcbALiberar);

        log_info(kernelLogger, "Largo Plazo: Se libera una instancia de Grado Multiprogramación");
        sem_post(&gradoMultiprog); /* Aumenta el grado de multiprogramación al tener carpincho en EXIT */
    }
}

static noreturn void* iniciar_largo_plazo(void* _) {
    pthread_t th;
    pthread_create(&th, NULL, liberar_carpinchos_en_exit, NULL);
    pthread_detach(th);
    log_info(kernelLogger, "Largo Plazo: Inicialización exitosa");
    for (;;) {
        /* tanto NEW como SUSREADY son parte del mismo conjunto: "el conjunto a pasar a READY" */
        sem_wait(&hayPCBsParaAgregarAlSistema);
        log_info(kernelLogger, "Largo Plazo: Se toma una instancia de PCBs a agregar al sistema");

        sem_wait(&gradoMultiprog);
        log_info(kernelLogger, "Largo Plazo: Se toma una instancia de Grado Multiprogramación");

        if (!list_is_empty(cola_planificacion_get_list(pcbsSusReady))) {
            sem_post(&transicionarSusReadyAready);
        } else {
            t_pcb* pcbQuePasaAReady = get_and_remove_first_pcb_from_queue(pcbsNew);

            pcb_algoritmo_init(pcbQuePasaAReady);
            log_info(kernelLogger, "Largo Plazo: Incialización de información del algoritmo correcta");

            pcb_set_status(pcbQuePasaAReady, READY);
            enqueue_pcb(pcbQuePasaAReady, pcbsReady);
            log_transition("Largo Plazo", "NEW", "READY", pcb_get_pid(pcbQuePasaAReady));

            sem_post(cola_planificacion_get_instancias_disponibles(pcbsReady));
        }
    }
}

static noreturn void* transicion_susready_a_ready(void* _) {
    log_info(kernelLogger, "Mediano Plazo: Hilo transicionador SUSP/READY->READY inicializado");
    for (;;) {
        sem_wait(&transicionarSusReadyAready);
        t_pcb* pcbQuePasaAReady = get_and_remove_first_pcb_from_queue(pcbsSusReady);
        pcb_set_status(pcbQuePasaAReady, READY);
        enqueue_pcb(pcbQuePasaAReady, pcbsReady);
        log_transition("Mediano Plazo", "SUSP/READY", "READY", pcb_get_pid(pcbQuePasaAReady));
        sem_post(cola_planificacion_get_instancias_disponibles(pcbsReady));
    }
}

static bool hay_procesos_en_esta_cola(t_cola_planificacion* cola) {
    pthread_mutex_t* mutex = cola_planificacion_get_mutex(cola);
    pthread_mutex_lock(mutex);
    bool existeProcesos = list_size(cola_planificacion_get_list(cola)) > 0;
    pthread_mutex_unlock(mutex);
    return existeProcesos;
}

static bool hay_procesos_en_bloqueados(void) {
    return hay_procesos_en_esta_cola(pcbsBlocked);
}

static bool hay_procesos_en_ready(void) {
    return hay_procesos_en_esta_cola(pcbsReady);
}

static bool hay_procesos_en_new(void) {
    return hay_procesos_en_esta_cola(pcbsNew);
}

static void evaluar_suspension(void) {
    sem_post(&despertarMedianoPlazo);
    sem_wait(&suspensionConcluida);
}

static int get_grado_multiprog_actual(void) {
    int gradoMultiprogActual;
    sem_getvalue(&gradoMultiprog, &gradoMultiprogActual);
    return gradoMultiprogActual;
}

static bool existe_caso_de_suspension(void) {
    return !hay_procesos_en_ready() && hay_procesos_en_new() && hay_procesos_en_bloqueados() && get_grado_multiprog_actual() == 0;
}

static t_pcb* pop_ultimo_de_cola(t_cola_planificacion* cola) {
    pthread_mutex_t* mutex = cola_planificacion_get_mutex(cola);
    pthread_mutex_lock(mutex);
    t_list* lista = cola_planificacion_get_list(cola);
    t_pcb* pcb = list_remove(lista, list_size(lista) - 1);
    pthread_mutex_unlock(mutex);
    return pcb;
}

static noreturn void* iniciar_mediano_plazo(void* _) {
    pthread_t th;
    pthread_create(&th, NULL, transicion_susready_a_ready, NULL);
    pthread_detach(th);
    for (;;) {
        sem_wait(&despertarMedianoPlazo);
        if (existe_caso_de_suspension()) {
            pthread_mutex_lock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
            t_pcb* pcbASuspender = pop_ultimo_de_cola(pcbsBlocked);
            enviar_suspension_de_carpincho_a_memoria(pcbASuspender);
            pcb_set_status(pcbASuspender, SUSBLOCKED);
            enqueue_pcb(pcbASuspender, pcbsSusBlocked);
            pthread_mutex_unlock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
            log_info(kernelLogger, "Mediano Plazo: Se libera una instancia de Grado Multiprogramación");
            log_transition("Mediano Plazo", "BLOCKED", "SUSP/BLOCKED", pcb_get_pid(pcbASuspender));
            sem_post(&gradoMultiprog); /* Aumenta el grado de multiprogramción al suspender a un proceso */
        }
        sem_post(&suspensionConcluida);
    }
}

static t_pcb* elegir_pcb_segun_algoritmo(t_cola_planificacion* cola) {
    t_pcb* pcb = NULL;
    if (pcb_is_sjf()) {
        pcb = elegir_en_base_a_sjf(cola);
    } else if (pcb_is_hrrn()) {
        pcb = elegir_en_base_a_hrrn(cola);
    }
    return pcb;
}

static t_tarea_sem* buffer_unpack_tarea_sem(t_buffer* buffer) {
    t_tarea_sem* peticionSem = malloc(sizeof(*peticionSem));
    peticionSem->nombre = buffer_unpack_string(buffer);

    return peticionSem;
}

static t_tarea_sem* buffer_unpack_tarea_sem_init(t_buffer* buffer) {
    t_tarea_sem* peticionSem = buffer_unpack_tarea_sem(buffer);
    buffer_unpack(buffer, &(peticionSem->valor), sizeof(peticionSem->valor));
    return peticionSem;
}

static t_tarea_call_io* buffer_unpack_tarea_call_io(t_buffer* buffer) {
    t_tarea_call_io* peticionCallIO = malloc(sizeof(*peticionCallIO));
    peticionCallIO->nombre = buffer_unpack_string(buffer);
    peticionCallIO->mensaje = buffer_unpack_string(buffer);
    return peticionCallIO;
}

static void kernel_sem_init(t_tarea_sem* unaTareaDeSem, t_pcb* pcb) {
    t_recurso_sem* recursoSem = recurso_sem_create(unaTareaDeSem->nombre, unaTareaDeSem->valor);
    pthread_mutex_t* mutex = cola_recursos_get_mutex(semaforosDelSistema);
    pthread_mutex_lock(mutex);
    list_add(cola_recursos_get_list(semaforosDelSistema), recursoSem);
    pthread_mutex_unlock(mutex);
}

static void tarea_sem_destroy(t_tarea_sem* unaTareaSem) {
    free(unaTareaSem->nombre);
    free(unaTareaSem);
}

static void tarea_call_IO_destroy(t_tarea_call_io* unaTareaCallIO) {
    free(unaTareaCallIO->mensaje);
    free(unaTareaCallIO->nombre);
    free(unaTareaCallIO);
}

static bool es_este_dispositivo_io(t_recurso_io* recursoIO, t_tarea_call_io* callIO) {
    return string_equals_ignore_case(recurso_io_get_nombre(recursoIO), callIO->nombre);
}

static void encolar_pcb_a_dispositivo_io(t_pcb* pcb, t_tarea_call_io* callIO) {
    t_recurso_io* recursoIO = list_find2(cola_recursos_get_list(dispositivosIODelSistema), (void*)es_este_dispositivo_io, callIO);

    if (recursoIO != NULL) {
        char* nombreDispositivo = string_from_format("Dispositivo I/O <%s>", recurso_io_get_nombre(recursoIO));

        dequeue_pcb(pcb, pcbsExec);

        pthread_mutex_lock(&mutexDeadlock);
        log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
        pcb_set_status(pcb, BLOCKED);
        enqueue_pcb(pcb, pcbsBlocked);
        pthread_mutex_unlock(&mutexDeadlock);
        log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);

        log_transition(nombreDispositivo, "EXEC", "BLOCKED", pcb_get_pid(pcb));
        log_info(kernelLogger, "%s: Se recibe mensaje \"%s\" de Carpincho ID %d", nombreDispositivo, callIO->mensaje, pcb_get_pid(pcb));
        free(nombreDispositivo);

        pthread_mutex_t* mutex = recurso_io_get_mutex_cola_pcbs(recursoIO);
        pthread_mutex_lock(mutex);
        queue_push(recurso_io_get_cola_pcbs(recursoIO), pcb);
        pthread_mutex_unlock(mutex);

        evaluar_suspension(); /* Caso posible en donde la cola de planificación Blocked tenga al menos 1 PCB */

        sem_post(recurso_io_get_sem_instancias_disponibles(recursoIO));
    } else {
        log_error(kernelLogger, "Dispositivo I/O <%s>: Recurso I/O no encontrado. Petición por Carpincho ID %d", callIO->nombre, pcb_get_pid(pcb));
    }
}

static bool realizar_tarea(t_buffer* buffer, uint32_t opCodeTarea, t_pcb* pcb) {
    uint32_t socket = *pcb_get_socket(pcb);

    t_tarea_sem* unaTareaSem = NULL;
    t_tarea_call_io* unaTareaCallIO = NULL;

    t_recurso_sem* sem = NULL;
    t_pcb* pcbDesbloqueado = NULL;

    bool esBloqueante = false;
    int index = -1;

    pthread_mutex_t* mutexSemsSist = cola_recursos_get_mutex(semaforosDelSistema);
    t_list* listaSemsSist = cola_recursos_get_list(semaforosDelSistema);

    switch (opCodeTarea) {
        case SEM_INIT:
            unaTareaSem = buffer_unpack_tarea_sem_init(buffer);
            pthread_mutex_lock(mutexSemsSist);
            sem = list_find2(listaSemsSist, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(mutexSemsSist);

            if (sem == NULL) {
                kernel_sem_init(unaTareaSem, pcb);
                log_info(kernelLogger, "SEM_INIT <Carpincho %d>: Inicialización semáforo \"%s\" con valor %d", pcb_get_pid(pcb), unaTareaSem->nombre, unaTareaSem->valor);
            } else {
                log_error(kernelLogger, "SEM_INIT <Carpincho %d>: Intento de inicialización semáforo \"%s\" ya existente en sistema", pcb_get_pid(pcb), unaTareaSem->nombre);
            }

            send_empty_buffer(OK_CONTINUE, socket);
            tarea_sem_destroy(unaTareaSem);
            return false;
        case SEM_WAIT:
            unaTareaSem = buffer_unpack_tarea_sem(buffer);
            pthread_mutex_lock(mutexSemsSist);
            sem = list_find2(listaSemsSist, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(mutexSemsSist);

            if (sem != NULL) {
                log_info(kernelLogger, "SEM_WAIT <Carpincho %d>: Valor semáforo: %d", pcb_get_pid(pcb), recurso_sem_get_valor_actual(sem));
                esBloqueante = kernel_sem_wait(sem, pcb);
                log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
                if (esBloqueante) {
                    log_info(kernelLogger, "SEM_WAIT <Carpincho %d>: Se bloquea en semáforo \"%s\". Valor semáforo: %d", pcb_get_pid(pcb), unaTareaSem->nombre, recurso_sem_get_valor_actual(sem));
                } else {
                    log_info(kernelLogger, "SEM_WAIT <Carpincho %d>: Continúa su ejecución. Valor semáforo: %d", pcb_get_pid(pcb), recurso_sem_get_valor_actual(sem));
                }
            } else {
                log_error(kernelLogger, "SEM_WAIT <Carpincho %d>: Semáforo %s no encontrado", pcb_get_pid(pcb), unaTareaSem->nombre);
            }

            send_empty_buffer(OK_CONTINUE, socket);
            tarea_sem_destroy(unaTareaSem);
            return esBloqueante;
        case SEM_POST:
            unaTareaSem = buffer_unpack_tarea_sem(buffer);
            pthread_mutex_lock(mutexSemsSist);
            sem = list_find2(listaSemsSist, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(mutexSemsSist);

            if (sem != NULL) {
                log_info(kernelLogger, "SEM_POST <Carpincho %d>: Valor semáforo: %d", pcb_get_pid(pcb), recurso_sem_get_valor_actual(sem));
                pthread_mutex_lock(&mutexDeadlock);
                log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
                pcbDesbloqueado = kernel_sem_post(sem, pcb);
                pthread_mutex_unlock(&mutexDeadlock);
                log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
                if (pcbDesbloqueado != NULL) {
                    log_info(kernelLogger, "SEM_POST <Carpincho %d>: Se desbloquea Carpincho %d en semáforo \"%s\". Valor semáforo: %d", pcb_get_pid(pcb), pcb_get_pid(pcbDesbloqueado), recurso_sem_get_nombre(sem), recurso_sem_get_valor_actual(sem));
                } else {
                    log_info(kernelLogger, "SEM_POST <Carpincho %d>: Ningún carpincho en cola de bloqueados de semáforo \"%s\". Valor semáforo: %d", pcb_get_pid(pcb), recurso_sem_get_nombre(sem), recurso_sem_get_valor_actual(sem));
                }
            } else {
                log_error(kernelLogger, "SEM_POST <Carpincho %d>: Semáforo \"%s\" no encontrado", pcb_get_pid(pcb), unaTareaSem->nombre);
            }

            send_empty_buffer(OK_CONTINUE, socket);
            tarea_sem_destroy(unaTareaSem);
            return false;
        case SEM_DESTROY:
            unaTareaSem = buffer_unpack_tarea_sem(buffer);
            pthread_mutex_lock(mutexSemsSist);
            sem = list_find2(listaSemsSist, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(mutexSemsSist);

            if (sem != NULL) {
                char* nombre = recurso_sem_get_nombre(sem);
                t_queue* colaBloqueados = recurso_sem_get_cola_pcbs(sem);
                if (queue_size(colaBloqueados) == 0) {
                    index = list_get_index(listaSemsSist, (void*)es_este_semaforo, (void*)nombre);
                    list_remove(listaSemsSist, index);
                    log_info(kernelLogger, "SEM_DESTROY <Carpincho %d>: Se elimina el semáforo \"%s\" con valor %d", pcb_get_pid(pcb), nombre, recurso_sem_get_valor_actual(sem));
                    recurso_sem_destroy(sem);
                } else {
                    log_info(kernelLogger, "SEM_DESTROY <Carpincho %d>: Denegación de eliminación de semáforo \"%s\". Cantidad de procesos bloqueados en semáforo: %d", pcb_get_pid(pcb), nombre, queue_size(colaBloqueados));
                }
            } else {
                log_error(kernelLogger, "SEM_DESTROY <Carpincho %d>: Semáforo %s no encontrado", pcb_get_pid(pcb), unaTareaSem->nombre);
            }

            send_empty_buffer(OK_CONTINUE, socket);
            tarea_sem_destroy(unaTareaSem);
            return false;
        case CALL_IO:
            unaTareaCallIO = buffer_unpack_tarea_call_io(buffer);
            encolar_pcb_a_dispositivo_io(pcb, unaTareaCallIO);
            send_empty_buffer(OK_CONTINUE, socket);
            tarea_call_IO_destroy(unaTareaCallIO);
            return true;
        case MEM_ALLOC:
            enviar_memalloc_a_memoria(pcb, buffer);
            log_info(kernelLogger, "Realizar Tarea: Saliendo de wrapper memalloc");
            return false;
        case MEM_FREE:
            enviar_memfree_a_memoria(pcb, buffer);
            log_info(kernelLogger, "Realizar Tarea: Saliendo de wrapper memfree");
            return false;
        case MEM_READ:
            enviar_memread_a_memoria(pcb, buffer);
            log_info(kernelLogger, "Realizar Tarea: Saliendo de wrapper memread");
            return false;
        case MEM_WRITE:
            enviar_memwrite_a_memoria(pcb, buffer);
            log_info(kernelLogger, "Realizar Tarea: Saliendo de wrapper memwrite");
            return false;
        default:
            log_error(kernelLogger, "Código de operación de tarea no conocida");
            return true;
    }
    return true;
}

static void atender_peticiones_del_carpincho(t_pcb* pcb) {
    bool peticionBloqueante = false;
    uint32_t socket = *pcb_get_socket(pcb);
    time_t start;
    time_t end;
    time(&start);
    while (!peticionBloqueante) {
        send_empty_buffer(OK_CONTINUE, socket);

        uint32_t opCodeTarea = get_op_code(socket);

        if (opCodeTarea == MATE_CLOSE) {
            recv_empty_buffer(socket);
            dequeue_pcb(pcb, pcbsExec);
            pcb_set_status(pcb, EXIT);
            enqueue_pcb(pcb, pcbsExit);
            log_transition("Corto Plazo", "EXEC", "EXIT", pcb_get_pid(pcb));
            sem_post(cola_planificacion_get_instancias_disponibles(pcbsExit));
            break;
        } else {
            t_buffer* buffer = buffer_create();
            get_buffer(socket, buffer);
            peticionBloqueante = realizar_tarea(buffer, opCodeTarea, pcb);
            if (peticionBloqueante) {
                time(&end);
                pcb_algoritmo_update_next_est_info(pcb, end, start);
            }
            buffer_destroy(buffer);
        }
    }
}

static noreturn void* iniciar_corto_plazo(void* _) {
    for (;;) {
        sem_wait(cola_planificacion_get_instancias_disponibles(pcbsReady));
        log_info(kernelLogger, "Corto Plazo: Se toma una instancia de READY");

        t_pcb* pcbQuePasaAExec = elegir_pcb_segun_algoritmo(pcbsReady);

        dequeue_pcb(pcbQuePasaAExec, pcbsReady);
        pcb_set_status(pcbQuePasaAExec, EXEC);
        enqueue_pcb(pcbQuePasaAExec, pcbsExec);

        log_transition("Corto Plazo", "READY", "EXEC", pcb_get_pid(pcbQuePasaAExec));
        evaluar_suspension(); /* Caso posible en donde la cola de planificación ready se quede sin PCBs */

        atender_peticiones_del_carpincho(pcbQuePasaAExec);
    }
}

static noreturn void* iniciar_rutina_deteccion_de_deadlocks(void* _) {
    log_info(kernelLogger, "Deadlock: Inicialización de análisis de deadlock exitosa");
    uint16_t tiempoDeadlock = kernel_config_get_tiempo_deadlock(kernelCfg);
    for (;;) {
        log_info(kernelLogger, "Deadlock: Durmiendo por %d milisegundos...", tiempoDeadlock);
        intervalo_de_pausa(tiempoDeadlock);
        log_info(kernelLogger, "Deadlock: Analizando deadlock...");
        pthread_mutex_lock(&mutexDeadlock);
        log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
        detectar_y_recuperarse_del_deadlock(pcbsBlocked, pcbsSusBlocked, semaforosDelSistema, &gradoMultiprog);
        pthread_mutex_unlock(&mutexDeadlock);
        log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
    }
}

void iniciar_planificacion(void) {
    nextPid = 1;

    sem_init(&gradoMultiprog, 0, kernel_config_get_grado_multiprog(kernelCfg)); /* contador */
    sem_init(&despertarMedianoPlazo, 0, 0);                                     /* binario  */
    sem_init(&suspensionConcluida, 0, 0);                                       /* binario  */
    sem_init(&hayPCBsParaAgregarAlSistema, 0, 0);                               /* contador */
    sem_init(&transicionarSusReadyAready, 0, 0);                                /* binario  */

    pthread_mutex_init(&mutexDeadlock, NULL);
    pthread_mutex_init(&mutexPid, NULL);
    pthread_mutex_init(&mutexMemSocket, NULL);

    /* Inicialización de recursos del sistema */
    semaforosDelSistema = cola_recursos_create();
    dispositivosIODelSistema = cola_recursos_create();

    /* Inicialización de dispositivos en TAD t_cola_recursos */
    inicializar_dispositivos_io(dispositivosIODelSistema);

    /* Inicialización de colas de planificación */
    pcbsNew = cola_planificacion_create(0);
    pcbsReady = cola_planificacion_create(0);
    pcbsBlocked = cola_planificacion_create(0);
    pcbsSusReady = cola_planificacion_create(0);
    pcbsSusBlocked = cola_planificacion_create(0);
    pcbsExit = cola_planificacion_create(0);
    pcbsExec = cola_planificacion_create(0);

    /* Hilos para ejecución de ráfagas de I/O */
    iniciar_ejecucion_lista_dispositivos_io();

    pthread_t th;

    /* Planificador largo plazo */
    pthread_create(&th, NULL, iniciar_largo_plazo, NULL);
    pthread_detach(th);

    /* Planificador mediano plazo */
    pthread_create(&th, NULL, iniciar_mediano_plazo, NULL);
    pthread_detach(th);

    /* Planificador corto plazo (N Hilos, siendo N el grado de multiprocesamiento) */
    uint16_t gradoMultiprocesamiento = kernel_config_get_grado_multiproc(kernelCfg);
    for (int i = 0; i < gradoMultiprocesamiento; i++) {
        pthread_create(&th, NULL, iniciar_corto_plazo, NULL);
        pthread_detach(th);
    }
    log_info(kernelLogger, "Corto Plazo: Inicialización de %d CPUs exitosa", gradoMultiprocesamiento);

    /* Hilo para detección de deadlocks */
    pthread_create(&th, NULL, iniciar_rutina_deteccion_de_deadlocks, NULL);
    pthread_detach(th);
}

void log_transition(const char* entityName, const char* prev, const char* post, int pid) {
    char* transicion = string_from_format("\e[1;93m%s->%s\e[0m", prev, post);
    log_info(kernelLogger, "%s: Transición de %s Carpincho ID %d", entityName, transicion, pid);
    free(transicion);
}

static uint32_t get_next_pid(void) {
    pthread_mutex_lock(&mutexPid);
    uint32_t pid = nextPid;
    pthread_mutex_unlock(&mutexPid);
    return pid;
}

static void pid_inc(uint32_t* pid) {
    pthread_mutex_lock(&mutexPid);
    (*pid)++;
    pthread_mutex_unlock(&mutexPid);
}

void* encolar_en_new_nuevo_carpincho_entrante(void* socketHilo) {
    uint32_t* socket = (uint32_t*)socketHilo;
    uint32_t opCodeTarea = get_op_code(*socket);
    recv_empty_buffer(*socket);

    if (opCodeTarea == MATE_INIT) {
        t_buffer* buffer = buffer_create();
        uint32_t siguientePid = get_next_pid();
        pid_inc(&nextPid);
        buffer_pack(buffer, &siguientePid, sizeof(siguientePid));
        buffer_send(buffer, OK_CONTINUE, *socket);
        t_pcb* pcb = pcb_create(socket, siguientePid);
        enqueue_pcb(pcb, pcbsNew);
        log_info(kernelLogger, "Kernel: Carpincho ID %d establece conexión", pcb_get_pid(pcb));
        log_info(kernelLogger, "Kernel: Creación PCB Carpincho ID %d exitosa", pcb_get_pid(pcb));

        evaluar_suspension(); /* Caso posible en donde la cola de planificación New tenga al menos 1 PCB */
        sem_post(&hayPCBsParaAgregarAlSistema);

        buffer_destroy(buffer);
    }
    pthread_exit(NULL);
}

t_pcb* elegir_en_base_a_sjf(t_cola_planificacion* colaPlanificacion) {
    pthread_mutex_t* mutex = cola_planificacion_get_mutex(colaPlanificacion);
    pthread_mutex_lock(mutex);
    t_pcb* pcbMenorEstimacion = (t_pcb*)list_get_minimum(cola_planificacion_get_list(colaPlanificacion), (void*)pcb_minimum_est);
    pthread_mutex_unlock(mutex);
    return pcbMenorEstimacion;
}

t_pcb* elegir_en_base_a_hrrn(t_cola_planificacion* colaPlanificacion) {
    pthread_mutex_t* mutex = cola_planificacion_get_mutex(colaPlanificacion);
    pthread_mutex_lock(mutex);
    time_t now;
    time(&now);
    t_pcb* pcbConMayorRR = list_get(cola_planificacion_get_list(colaPlanificacion), 0);
    t_pcb* pcbTemp = NULL;
    t_list* listaPCBsReady = cola_planificacion_get_list(pcbsReady);
    for (int i = 1; i < list_size(listaPCBsReady); i++) {
        pcbTemp = list_get(listaPCBsReady, i);
        double supuestoMayor = response_ratio(pcbConMayorRR, now);
        double tempRR = response_ratio(pcbTemp, now);
        if (tempRR > supuestoMayor) {
            pcbConMayorRR = pcbTemp;
        }
    }
    pthread_mutex_unlock(mutex);
    return pcbConMayorRR;
}

static void encolar_pcb_al_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    pthread_mutex_t* mutexColaPCBs = recurso_sem_get_mutex_cola_pcbs(sem);
    pthread_mutex_lock(mutexColaPCBs);
    t_queue* colaPCBs = recurso_sem_get_cola_pcbs(sem);
    queue_push(colaPCBs, pcb);
    pthread_mutex_unlock(mutexColaPCBs);
}

static t_pcb* pop_primer_pcb_de_cola_semaforo(t_recurso_sem* sem) {
    t_pcb* primerPCB = NULL;
    pthread_mutex_t* mutexColaPCBs = recurso_sem_get_mutex_cola_pcbs(sem);
    pthread_mutex_lock(mutexColaPCBs);
    t_queue* colaPCBs = recurso_sem_get_cola_pcbs(sem);
    primerPCB = queue_pop(colaPCBs);
    pthread_mutex_unlock(mutexColaPCBs);
    return primerPCB;
}

void retener_una_instancia_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    int32_t* valorActual = NULL;
    t_deadlock* info = pcb_get_deadlock_info(pcb);
    pthread_mutex_lock(deadlock_get_dict_mutex(info));
    t_dictionary* dict = deadlock_get_dict(info);
    char* nombre = recurso_sem_get_nombre(sem);
    if (dictionary_has_key(dict, nombre)) {
        valorActual = (int32_t*)dictionary_get(dict, nombre);
        (*valorActual)++;
    } else {
        valorActual = malloc(sizeof(*valorActual));
        *valorActual = 1;
    }
    dictionary_put(dict, nombre, valorActual);
    pthread_mutex_unlock(deadlock_get_dict_mutex(info));
}

void liberar_una_instancia_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    t_deadlock* info = pcb_get_deadlock_info(pcb);
    pthread_mutex_lock(deadlock_get_dict_mutex(info));
    t_dictionary* dict = deadlock_get_dict(info);
    int32_t* valorActual = NULL;
    char* nombre = recurso_sem_get_nombre(sem);
    if (dictionary_has_key(dict, nombre)) {
        valorActual = (int32_t*)dictionary_get(dict, nombre);
        (*valorActual)--;
        dictionary_put(dict, nombre, valorActual);
        if (*valorActual == 0) {
            valorActual = dictionary_remove(dict, nombre);
            free(valorActual);
        }
    }
    pthread_mutex_unlock(deadlock_get_dict_mutex(info));
}

bool kernel_sem_wait(t_recurso_sem* sem, t_pcb* pcbWait) {
    pthread_mutex_t* mutexValorSem = recurso_sem_get_mutex_valor_sem(sem);
    pthread_mutex_lock(mutexValorSem);
    recurso_sem_dec_cant_instancias(sem);
    bool esBloqueante = recurso_sem_es_bloqueante(sem);
    pthread_mutex_unlock(mutexValorSem);

    if (esBloqueante) {
        dequeue_pcb(pcbWait, pcbsExec);

        pcb_set_status(pcbWait, BLOCKED);
        deadlock_set_semaforo_en_que_espera(pcb_get_deadlock_info(pcbWait), sem);
        encolar_pcb_al_semaforo(pcbWait, sem);
        enqueue_pcb(pcbWait, pcbsBlocked);

        log_transition("Kernel", "EXEC", "BLOCKED", pcb_get_pid(pcbWait));

        evaluar_suspension(); /* Caso posible en donde la cola de planificación Blocked tenga al menos 1 PCB */
    } else {
        retener_una_instancia_del_semaforo(pcbWait, sem);
    }
    return esBloqueante;
}

t_pcb* kernel_sem_post(t_recurso_sem* sem, t_pcb* pcbPost) {
    t_pcb* primerPCB = NULL;
    pthread_mutex_t* mutexValorSem = recurso_sem_get_mutex_valor_sem(sem);
    pthread_mutex_lock(mutexValorSem);
    recurso_sem_inc_cant_instancias(sem);
    bool hayPCBsBloqueados = recurso_sem_hay_pcbs_bloqueados(sem);
    pthread_mutex_unlock(mutexValorSem);

    liberar_una_instancia_del_semaforo(pcbPost, sem);

    if (hayPCBsBloqueados) {
        primerPCB = pop_primer_pcb_de_cola_semaforo(sem);
        if (primerPCB != NULL) {
            t_status pcbCurrStatus = pcb_get_status(primerPCB);
            if (pcbCurrStatus == SUSBLOCKED) { /* Viene de SUSBLOCKED => ya lo habíamos eliminado de la cola general de bloqueados al momento de suspenderlo */
                pasar_de_susblocked_a_susready(primerPCB);
            } else if (pcbCurrStatus == BLOCKED) { /* Viene de BLOCKED => hay que eliminarlo de la lista general de bloqueados */
                pasar_de_blocked_a_ready(primerPCB);
            }
            deadlock_set_semaforo_en_que_espera(pcb_get_deadlock_info(primerPCB), NULL);
            retener_una_instancia_del_semaforo(primerPCB, sem);
        }
    }
    return primerPCB;
}
