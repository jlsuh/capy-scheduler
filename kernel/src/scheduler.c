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

static int pcb_get_posicion(t_pcb* pcb, t_list* lista) {
    for (int posicion = 0; posicion < list_size(lista); posicion++) {
        if (pcb == (t_pcb*)list_get(lista, posicion)) {
            return posicion;
        }
    }
    return -1;
}

static void agregar_pcb_a_cola(t_pcb* pcb, t_cola_planificacion* cola) {
    pthread_mutex_lock(&(cola->mutex));
    list_add(cola->lista, pcb);
    pthread_mutex_unlock(&(cola->mutex));
}

static void remover_pcb_de_cola(t_pcb* pcb, t_cola_planificacion* cola) {
    pthread_mutex_lock(&(cola->mutex));
    int posicion = pcb_get_posicion(pcb, cola->lista);
    if (posicion != -1) {
        list_remove(cola->lista, posicion);
    } else {
        log_error(kernelLogger, "Kernel: No existe tal elemento en la cola");
    }
    pthread_mutex_unlock(&(cola->mutex));
}

static void cambiar_estado_pcb(t_pcb* pcb, t_status nuevoEstado) {
    pcb->status = nuevoEstado;
}

static void pasar_de_susblocked_a_susready(t_pcb* pcb) {
    remover_pcb_de_cola(pcb, pcbsSusBlocked);
    cambiar_estado_pcb(pcb, SUSREADY);
    agregar_pcb_a_cola(pcb, pcbsSusReady);
    sem_post(&hayPCBsParaAgregarAlSistema);
}

static void pasar_de_blocked_a_ready(t_pcb* pcb) {
    remover_pcb_de_cola(pcb, pcbsBlocked);
    cambiar_estado_pcb(pcb, READY);
    agregar_pcb_a_cola(pcb, pcbsReady);
    sem_post(&(pcbsReady->instanciasDisponibles));
}

static t_cola_planificacion* cola_planificacion_create(int semInitVal) {
    t_cola_planificacion* self = malloc(sizeof(*self));
    self->lista = list_create();
    pthread_mutex_init(&(self->mutex), NULL);
    sem_init(&(self->instanciasDisponibles), 0, semInitVal);
    return self;
}

static t_recurso_io* recurso_io_create(char* nombre, uint32_t duracion) {
    t_recurso_io* recursoIO = malloc(sizeof(*recursoIO));
    recursoIO->colaPCBs = queue_create();
    recursoIO->nombre = nombre;
    recursoIO->duracion = duracion;
    sem_init(&(recursoIO->instanciasDisponibles), 0, 0);
    pthread_mutex_init(&(recursoIO->mutexColaPCBs), NULL);
    return recursoIO;
}

static void inicializar_dispositivos_io_config(t_cola_recursos* dispositivosIODelSistema) {
    uint32_t duracionDispositivo;
    t_recurso_io* recursoIO;
    char** duracionesIO = kernel_config_get_duracionesIO(kernelCfg);
    char** dispositivosIO = kernel_config_get_dispositivosIO(kernelCfg);
    for (int i = 0; i < string_array_size(duracionesIO); i++) {
        duracionDispositivo = atoi(duracionesIO[i]);
        recursoIO = recurso_io_create(dispositivosIO[i], duracionDispositivo);
        list_add(dispositivosIODelSistema->listaRecursos, recursoIO);
        log_info(kernelLogger, "Kernel: Inicialización de recurso %s con duración de I/O de %d milisegundos", recursoIO->nombre, recursoIO->duracion);
    }
}

static void ejecutar_rafagas_io(t_recurso_io* recursoIO, t_pcb* primerPCB) {
    log_info(kernelLogger, "Dispositivo I/O <%s>: Carpincho ID %d ejecutando ráfagas I/O", recursoIO->nombre, primerPCB->pid);
    intervalo_de_pausa(recursoIO->duracion);
    log_info(kernelLogger, "Dispositivo I/O <%s>: Fin ráfagas I/O Carpincho ID %d", recursoIO->nombre, primerPCB->pid);
}

static noreturn void* iniciar_rutina_ejecucion_dispositivos_io(void* args) {
    t_recurso_io* recursoIO = (t_recurso_io*)args;
    for (;;) {
        char* nombreDispositivo = string_from_format("Dispositivo I/O <%s>", recursoIO->nombre);

        sem_wait(&(recursoIO->instanciasDisponibles));

        pthread_mutex_lock(&(recursoIO->mutexColaPCBs));
        t_pcb* primerPCB = queue_pop(recursoIO->colaPCBs);
        pthread_mutex_unlock(&(recursoIO->mutexColaPCBs));

        ejecutar_rafagas_io(recursoIO, primerPCB);

        if (primerPCB->status == SUSBLOCKED) {
            pthread_mutex_lock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
            pasar_de_susblocked_a_susready(primerPCB);
            pthread_mutex_unlock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
            log_transition(nombreDispositivo, "SUSP/BLOCKED", "SUSP/READY", primerPCB->pid);
        } else if (primerPCB->status == BLOCKED) {
            pthread_mutex_lock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
            pasar_de_blocked_a_ready(primerPCB);
            pthread_mutex_unlock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
            log_transition(nombreDispositivo, "BLOCKED", "READY", primerPCB->pid);
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
    list_iterate(dispositivosIODelSistema->listaRecursos, (void*)iniciar_dispositivo_io);
}

static t_pcb* get_and_remove_primer_pcb_de_cola(t_cola_planificacion* cola) {
    t_pcb* pcb = NULL;
    pthread_mutex_lock(&(cola->mutex));
    if (!list_is_empty(cola->lista)) {
        pcb = (t_pcb*)list_remove(cola->lista, 0);
    }
    pthread_mutex_unlock(&(cola->mutex));
    return pcb;
}

static noreturn void* liberar_carpinchos_en_exit(void* _) {
    log_info(kernelLogger, "Largo Plazo: Hilo liberador de PCBs de Carpinchos en EXIT inicializado");
    for (;;) {
        sem_wait(&(pcbsExit->instanciasDisponibles));

        t_pcb* pcbALiberar = get_and_remove_primer_pcb_de_cola(pcbsExit);

        enviar_mate_close_a_memoria(pcbALiberar);

        send_empty_buffer(OK_FINISH, *(pcbALiberar->socket));
        log_info(kernelLogger, "Kernel: Desconexión Carpincho ID %d", pcbALiberar->pid);
        pcbALiberar->algoritmo_destroy(pcbALiberar);

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

        if (!list_is_empty(pcbsSusReady->lista)) {
            sem_post(&transicionarSusReadyAready);
        } else {
            t_pcb* pcbQuePasaAReady = get_and_remove_primer_pcb_de_cola(pcbsNew);
            pcbQuePasaAReady->algoritmo_init(pcbQuePasaAReady);
            log_info(kernelLogger, "Largo Plazo: Incialización de información del algoritmo correcta");

            cambiar_estado_pcb(pcbQuePasaAReady, READY);
            agregar_pcb_a_cola(pcbQuePasaAReady, pcbsReady);
            log_transition("Largo Plazo", "NEW", "READY", pcbQuePasaAReady->pid);

            sem_post(&(pcbsReady->instanciasDisponibles));
        }
    }
}

static noreturn void* transicion_susready_a_ready(void* _) {
    log_info(kernelLogger, "Mediano Plazo: Hilo transicionador SUSP/READY->READY inicializado");
    for (;;) {
        sem_wait(&transicionarSusReadyAready);
        t_pcb* pcbQuePasaAReady = get_and_remove_primer_pcb_de_cola(pcbsSusReady);
        cambiar_estado_pcb(pcbQuePasaAReady, READY);
        agregar_pcb_a_cola(pcbQuePasaAReady, pcbsReady);
        log_transition("Mediano Plazo", "SUSP/READY", "READY", pcbQuePasaAReady->pid);
        sem_post(&(pcbsReady->instanciasDisponibles));
    }
}

static bool hay_procesos_en_esta_cola(t_cola_planificacion* cola) {
    pthread_mutex_lock(&(cola->mutex));
    bool existeProcesos = list_size(cola->lista) > 0;
    pthread_mutex_unlock(&(cola->mutex));
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
    pthread_mutex_lock(&(cola->mutex));
    t_pcb* pcb = list_remove(cola->lista, list_size(cola->lista) - 1);
    pthread_mutex_unlock(&(cola->mutex));
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
            cambiar_estado_pcb(pcbASuspender, SUSBLOCKED);
            agregar_pcb_a_cola(pcbASuspender, pcbsSusBlocked);
            pthread_mutex_unlock(&mutexDeadlock);
            log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
            log_info(kernelLogger, "Mediano Plazo: Se libera una instancia de Grado Multiprogramación");
            log_transition("Mediano Plazo", "BLOCKED", "SUSP/BLOCKED", pcbASuspender->pid);
            sem_post(&gradoMultiprog); /* Aumenta el grado de multiprogramción al suspender a un proceso */
        }
        sem_post(&suspensionConcluida);
    }
}

static bool algoritmo_sjf_loaded(void) {
    return strcmp(kernel_config_get_algoritmo_planificacion(kernelCfg), "SJF") == 0;
}

static bool algoritmo_hrrn_loaded(void) {
    return strcmp(kernel_config_get_algoritmo_planificacion(kernelCfg), "HRRN") == 0;
}

double get_diferencial_de_tiempo(time_t tiempoFinal, time_t tiempoInicial) {
    double diferencialT = difftime(tiempoFinal, tiempoInicial);
    return diferencialT;
}

double media_exponencial(double realAnterior, double estAnterior) {
    /* Est(n) = α . R(n-1) + (1 - α) . Est(n-1) */
    double const alfa = kernel_config_get_alfa(kernelCfg);
    return alfa * realAnterior + (1 - alfa) * estAnterior;
}

static t_pcb* elegir_pcb_segun_algoritmo(t_cola_planificacion* cola) {
    t_pcb* pcb = NULL;
    if (algoritmo_sjf_loaded()) {
        pcb = elegir_en_base_a_sjf(cola);
    } else if (algoritmo_hrrn_loaded()) {
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
    pthread_mutex_lock(&(semaforosDelSistema->mutexRecursos));
    list_add(semaforosDelSistema->listaRecursos, recursoSem);
    pthread_mutex_unlock(&(semaforosDelSistema->mutexRecursos));
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
    return string_equals_ignore_case(recursoIO->nombre, callIO->nombre);
}

static void encolar_pcb_a_dispositivo_io(t_pcb* pcb, t_tarea_call_io* callIO) {
    t_recurso_io* recursoIO = list_find2(dispositivosIODelSistema->listaRecursos, (void*)es_este_dispositivo_io, callIO);

    if (recursoIO != NULL) {
        char* nombreDispositivo = string_from_format("Dispositivo I/O <%s>", recursoIO->nombre);

        remover_pcb_de_cola(pcb, pcbsExec);

        pthread_mutex_lock(&mutexDeadlock);
        log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
        cambiar_estado_pcb(pcb, BLOCKED);
        agregar_pcb_a_cola(pcb, pcbsBlocked);
        pthread_mutex_unlock(&mutexDeadlock);
        log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);

        log_transition(nombreDispositivo, "EXEC", "BLOCKED", pcb->pid);
        log_info(kernelLogger, "%s: Se recibe mensaje \"%s\" de Carpincho ID %d", nombreDispositivo, callIO->mensaje, pcb->pid);
        free(nombreDispositivo);

        pthread_mutex_lock(&(recursoIO->mutexColaPCBs));
        queue_push(recursoIO->colaPCBs, pcb);
        pthread_mutex_unlock(&(recursoIO->mutexColaPCBs));

        evaluar_suspension(); /* Caso posible en donde la cola de planificación Blocked tenga al menos 1 PCB */

        sem_post(&(recursoIO->instanciasDisponibles));
    } else {
        log_error(kernelLogger, "Dispositivo I/O <%s>: Recurso I/O no encontrado. Petición por Carpincho ID %d", callIO->nombre, pcb->pid);
    }
}

static bool realizar_tarea(t_buffer* buffer, uint32_t opCodeTarea, t_pcb* pcb) {
    uint32_t socket = *(pcb->socket);

    t_tarea_sem* unaTareaSem = NULL;
    t_tarea_call_io* unaTareaCallIO = NULL;

    t_recurso_sem* sem = NULL;
    t_pcb* pcbDesbloqueado = NULL;

    bool esBloqueante = false;
    int index = -1;

    switch (opCodeTarea) {
        case SEM_INIT:
            unaTareaSem = buffer_unpack_tarea_sem_init(buffer);
            pthread_mutex_lock(&(semaforosDelSistema->mutexRecursos));
            sem = list_find2(semaforosDelSistema->listaRecursos, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(&(semaforosDelSistema->mutexRecursos));

            if (sem == NULL) {
                kernel_sem_init(unaTareaSem, pcb);
                log_info(kernelLogger, "SEM_INIT <Carpincho %d>: Inicialización semáforo \"%s\" con valor %d", pcb->pid, unaTareaSem->nombre, unaTareaSem->valor);
            } else {
                log_error(kernelLogger, "SEM_INIT <Carpincho %d>: Intento de inicialización semáforo \"%s\" ya existente en sistema", pcb->pid, unaTareaSem->nombre);
            }

            send_empty_buffer(OK_CONTINUE, socket);
            tarea_sem_destroy(unaTareaSem);
            return false;
        case SEM_WAIT:
            unaTareaSem = buffer_unpack_tarea_sem(buffer);
            pthread_mutex_lock(&(semaforosDelSistema->mutexRecursos));
            sem = list_find2(semaforosDelSistema->listaRecursos, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(&(semaforosDelSistema->mutexRecursos));

            if (sem != NULL) {
                log_info(kernelLogger, "SEM_WAIT <Carpincho %d>: Valor semáforo: %d", pcb->pid, sem->valorActual);
                esBloqueante = kernel_sem_wait(sem, pcb);
                log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
                if (esBloqueante) {
                    log_info(kernelLogger, "SEM_WAIT <Carpincho %d>: Se bloquea en semáforo \"%s\". Valor semáforo: %d", pcb->pid, unaTareaSem->nombre, sem->valorActual);
                } else {
                    log_info(kernelLogger, "SEM_WAIT <Carpincho %d>: Continúa su ejecución. Valor semáforo: %d", pcb->pid, sem->valorActual);
                }
            } else {
                log_error(kernelLogger, "SEM_WAIT <Carpincho %d>: Semáforo %s no encontrado", pcb->pid, unaTareaSem->nombre);
            }

            send_empty_buffer(OK_CONTINUE, socket);
            tarea_sem_destroy(unaTareaSem);
            return esBloqueante;
        case SEM_POST:
            unaTareaSem = buffer_unpack_tarea_sem(buffer);
            pthread_mutex_lock(&(semaforosDelSistema->mutexRecursos));
            sem = list_find2(semaforosDelSistema->listaRecursos, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(&(semaforosDelSistema->mutexRecursos));

            if (sem != NULL) {
                log_info(kernelLogger, "SEM_POST <Carpincho %d>: Valor semáforo: %d", pcb->pid, sem->valorActual);
                pthread_mutex_lock(&mutexDeadlock);
                log_info(kernelLogger, "Deadlock: Se toma el mutexDeadlock en línea %d", __LINE__);
                pcbDesbloqueado = kernel_sem_post(sem, pcb);
                pthread_mutex_unlock(&mutexDeadlock);
                log_info(kernelLogger, "Deadlock: Se libera el mutexDeadlock en línea %d", __LINE__);
                if (pcbDesbloqueado != NULL) {
                    log_info(kernelLogger, "SEM_POST <Carpincho %d>: Se desbloquea Carpincho %d en semáforo \"%s\". Valor semáforo: %d", pcb->pid, pcbDesbloqueado->pid, sem->nombre, sem->valorActual);
                } else {
                    log_info(kernelLogger, "SEM_POST <Carpincho %d>: Ningún carpincho en cola de bloqueados de semáforo \"%s\". Valor semáforo: %d", pcb->pid, sem->nombre, sem->valorActual);
                }
            } else {
                log_error(kernelLogger, "SEM_POST <Carpincho %d>: Semáforo \"%s\" no encontrado", pcb->pid, unaTareaSem->nombre);
            }

            send_empty_buffer(OK_CONTINUE, socket);
            tarea_sem_destroy(unaTareaSem);
            return false;
        case SEM_DESTROY:
            unaTareaSem = buffer_unpack_tarea_sem(buffer);
            pthread_mutex_lock(&(semaforosDelSistema->mutexRecursos));
            sem = list_find2(semaforosDelSistema->listaRecursos, (void*)es_este_semaforo, unaTareaSem->nombre);
            pthread_mutex_unlock(&(semaforosDelSistema->mutexRecursos));

            if (sem != NULL) {
                if (queue_size(sem->colaPCBs) == 0) {
                    index = list_get_index(semaforosDelSistema->listaRecursos, (void*)es_este_semaforo, (void*)sem->nombre);
                    list_remove(semaforosDelSistema->listaRecursos, index);
                    log_info(kernelLogger, "SEM_DESTROY <Carpincho %d>: Se elimina el semáforo \"%s\" con valor %d", pcb->pid, sem->nombre, sem->valorActual);
                    recurso_sem_destroy(sem);
                } else {
                    log_info(kernelLogger, "SEM_DESTROY <Carpincho %d>: Denegación de eliminación de semáforo \"%s\". Cantidad de procesos bloqueados en semáforo: %d", pcb->pid, sem->nombre, queue_size(sem->colaPCBs));
                }
            } else {
                log_error(kernelLogger, "SEM_DESTROY <Carpincho %d>: Semáforo %s no encontrado", pcb->pid, unaTareaSem->nombre);
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
    uint32_t socket = *(pcb->socket);
    time_t start;
    time_t end;
    time(&start);
    while (!peticionBloqueante) {
        send_empty_buffer(OK_CONTINUE, socket);

        uint32_t opCodeTarea = get_op_code(socket);

        if (opCodeTarea == MATE_CLOSE) {
            recv_empty_buffer(socket);
            remover_pcb_de_cola(pcb, pcbsExec);
            cambiar_estado_pcb(pcb, EXIT);
            agregar_pcb_a_cola(pcb, pcbsExit);
            log_transition("Corto Plazo", "EXEC", "EXIT", pcb->pid);
            sem_post(&(pcbsExit->instanciasDisponibles));
            break;
        } else {
            t_buffer* buffer = buffer_create();
            get_buffer(socket, buffer);
            peticionBloqueante = realizar_tarea(buffer, opCodeTarea, pcb);
            if (peticionBloqueante) {
                time(&end);
                pcb->algoritmo_update_next_est_info(pcb, end, start);
            }
            buffer_destroy(buffer);
        }
    }
}

static noreturn void* iniciar_corto_plazo(void* _) {
    for (;;) {
        sem_wait(&(pcbsReady->instanciasDisponibles));
        log_info(kernelLogger, "Corto Plazo: Se toma una instancia de READY");

        t_pcb* pcbQuePasaAExec = elegir_pcb_segun_algoritmo(pcbsReady);

        remover_pcb_de_cola(pcbQuePasaAExec, pcbsReady);
        cambiar_estado_pcb(pcbQuePasaAExec, EXEC);
        agregar_pcb_a_cola(pcbQuePasaAExec, pcbsExec);

        log_transition("Corto Plazo", "READY", "EXEC", pcbQuePasaAExec->pid);
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
    inicializar_dispositivos_io_config(dispositivosIODelSistema);

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

static uint32_t get_siguiente_pid(void) {
    pthread_mutex_lock(&mutexPid);
    uint32_t pid = nextPid;
    nextPid++;
    pthread_mutex_unlock(&mutexPid);
    return pid;
}

void* encolar_en_new_nuevo_carpincho_entrante(void* socketHilo) {
    uint32_t* socket = (uint32_t*)socketHilo;
    uint32_t opCodeTarea = get_op_code(*socket);
    recv_empty_buffer(*socket);

    if (opCodeTarea == MATE_INIT) {
        t_buffer* buffer = buffer_create();
        uint32_t siguientePid = get_siguiente_pid();
        buffer_pack(buffer, &siguientePid, sizeof(siguientePid));
        buffer_send(buffer, OK_CONTINUE, *socket);
        t_pcb* pcb = pcb_create(socket, siguientePid);
        agregar_pcb_a_cola(pcb, pcbsNew);
        log_info(kernelLogger, "Kernel: Carpincho ID %d establece conexión", pcb->pid);
        log_info(kernelLogger, "Kernel: Creación PCB Carpincho ID %d exitosa", pcb->pid);

        evaluar_suspension(); /* Caso posible en donde la cola de planificación New tenga al menos 1 PCB */
        sem_post(&hayPCBsParaAgregarAlSistema);

        buffer_destroy(buffer);
    }
    pthread_exit(NULL);
}

t_pcb* elegir_en_base_a_sjf(t_cola_planificacion* colaPlanificacion) {
    pthread_mutex_lock(&(colaPlanificacion->mutex));
    t_pcb* pcbMenorEstimacion = (t_pcb*)list_get_minimum(colaPlanificacion->lista, (void*)sjf_pcb_menor_estimacion_entre);
    pthread_mutex_unlock(&(colaPlanificacion->mutex));
    return pcbMenorEstimacion;
}

t_pcb* sjf_pcb_menor_estimacion_entre(t_pcb* unPcb, t_pcb* otroPcb) {
    return unPcb->sjf->estActual <= otroPcb->sjf->estActual ? unPcb : otroPcb;
}

void sjf_actualizar_info_para_siguiente_estimacion(t_pcb* pcb, time_t tiempoFinal, time_t tiempoInicial) {
    double realAnterior = get_diferencial_de_tiempo(tiempoFinal, tiempoInicial);
    pcb->sjf->estActual = media_exponencial(realAnterior, pcb->sjf->estActual);
}

void inicializar_sjf(t_pcb* pcb) {
    pcb->sjf = malloc(sizeof(*(pcb->sjf)));
    pcb->sjf->estActual = kernel_config_get_est_inicial(kernelCfg);
}

void inicializar_hrrn(t_pcb* pcb) {
    pcb->hrrn = malloc(sizeof(*(pcb->hrrn)));
    pcb->hrrn->s = kernel_config_get_est_inicial(kernelCfg);
    time(&(pcb->hrrn->w));
}

void hrrn_actualizar_info_para_siguiente_estimacion(t_pcb* pcb, time_t tiempoFinal, time_t tiempoInicial) {
    double realAnterior = get_diferencial_de_tiempo(tiempoFinal, tiempoInicial);
    pcb->hrrn->s = media_exponencial(realAnterior, pcb->hrrn->s);
    time(&(pcb->hrrn->w));
}

t_pcb* elegir_en_base_a_hrrn(t_cola_planificacion* colaPlanificacion) {
    pthread_mutex_lock(&(colaPlanificacion->mutex));
    time_t now;
    time(&now);
    t_pcb* pcbConMayorRR = list_get(colaPlanificacion->lista, 0);
    t_pcb* pcbTemp = NULL;
    for (int i = 1; i < list_size(pcbsReady->lista); i++) {
        pcbTemp = list_get(pcbsReady->lista, i);
        double supuestoMayor = response_ratio(pcbConMayorRR, now);
        double tempRR = response_ratio(pcbTemp, now);
        if (tempRR > supuestoMayor) {
            pcbConMayorRR = pcbTemp;
        }
    }
    pthread_mutex_unlock(&(colaPlanificacion->mutex));

    return pcbConMayorRR;
}

double response_ratio(t_pcb* pcb, time_t now) {
    return get_diferencial_de_tiempo(now, pcb->hrrn->w) / pcb->hrrn->s + 1;
}

t_pcb* pcb_create(uint32_t* socket, uint32_t pid) {
    t_pcb* self = malloc(sizeof(*self));
    self->socket = socket;
    self->pid = pid;
    self->status = NEW;
    self->sjf = NULL;
    self->hrrn = NULL;
    if (algoritmo_sjf_loaded()) {
        self->algoritmo_init = inicializar_sjf;
        self->algoritmo_destroy = sjf_destroy;
        self->algoritmo_update_next_est_info = sjf_actualizar_info_para_siguiente_estimacion;
    } else if (algoritmo_hrrn_loaded()) {
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

t_cola_recursos* cola_recursos_create(void) {
    t_cola_recursos* colaRecursos = malloc(sizeof(*colaRecursos));
    colaRecursos->listaRecursos = list_create();
    pthread_mutex_init(&(colaRecursos->mutexRecursos), NULL);
    return colaRecursos;
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

static void encolar_pcb_al_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    pthread_mutex_lock(&(sem->mutexColaPCBs));
    queue_push(sem->colaPCBs, pcb);
    pthread_mutex_unlock(&(sem->mutexColaPCBs));
}

static t_pcb* pop_primer_pcb_de_cola_semaforo(t_recurso_sem* sem) {
    t_pcb* primerPCB = NULL;
    pthread_mutex_lock(&(sem->mutexColaPCBs));
    primerPCB = queue_pop(sem->colaPCBs);
    pthread_mutex_unlock(&(sem->mutexColaPCBs));
    return primerPCB;
}

void retener_una_instancia_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    int32_t* valorActual = NULL;
    pthread_mutex_lock(&(pcb->deadlockInfo->mutexDict));
    if (dictionary_has_key(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre)) {
        valorActual = (int32_t*)dictionary_get(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre);
        (*valorActual)++;
    } else {
        valorActual = malloc(sizeof(*valorActual));
        *valorActual = 1;
    }
    dictionary_put(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre, valorActual);
    pthread_mutex_unlock(&(pcb->deadlockInfo->mutexDict));
}

bool kernel_sem_wait(t_recurso_sem* sem, t_pcb* pcbWait) {
    pthread_mutex_lock(&(sem->mutexValorSemaforo));
    sem->valorActual--;
    bool esBloqueante = sem->valorActual < 0;
    pthread_mutex_unlock(&(sem->mutexValorSemaforo));

    if (esBloqueante) {
        remover_pcb_de_cola(pcbWait, pcbsExec);

        cambiar_estado_pcb(pcbWait, BLOCKED);
        pcbWait->deadlockInfo->esperaEnSemaforo = sem;
        encolar_pcb_al_semaforo(pcbWait, sem);
        agregar_pcb_a_cola(pcbWait, pcbsBlocked);

        log_transition("Kernel", "EXEC", "BLOCKED", pcbWait->pid);

        evaluar_suspension(); /* Caso posible en donde la cola de planificación Blocked tenga al menos 1 PCB */
    } else {
        retener_una_instancia_del_semaforo(pcbWait, sem);
    }
    return esBloqueante;
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

t_pcb* kernel_sem_post(t_recurso_sem* sem, t_pcb* pcbPost) {
    t_pcb* primerPCB = NULL;

    pthread_mutex_lock(&(sem->mutexValorSemaforo));
    sem->valorActual++;
    bool hayPCBsBloqueados = sem->valorActual <= 0;
    pthread_mutex_unlock(&(sem->mutexValorSemaforo));

    liberar_una_instancia_del_semaforo(pcbPost, sem);

    if (hayPCBsBloqueados) {
        primerPCB = pop_primer_pcb_de_cola_semaforo(sem);
        if (primerPCB != NULL) {
            if (primerPCB->status == SUSBLOCKED) { /* Viene de SUSBLOCKED => ya lo habíamos eliminado de la cola general de bloqueados al momento de suspenderlo */
                pasar_de_susblocked_a_susready(primerPCB);
            } else if (primerPCB->status == BLOCKED) { /* Viene de BLOCKED => hay que eliminarlo de la lista general de bloqueados */
                pasar_de_blocked_a_ready(primerPCB);
            }
            primerPCB->deadlockInfo->esperaEnSemaforo = NULL;
            retener_una_instancia_del_semaforo(primerPCB, sem);
        }
    }
    return primerPCB;
}

void pcb_destroy(t_pcb* pcb) {
    free(pcb->socket);
    dictionary_destroy_and_destroy_elements(pcb->deadlockInfo->semaforosQueRetiene, free);
    free(pcb->deadlockInfo);
    free(pcb);
}

void recurso_sem_destroy(t_recurso_sem* unSemaforo) {
    queue_destroy(unSemaforo->colaPCBs);
    pthread_mutex_destroy(&(unSemaforo->mutexColaPCBs));
    pthread_mutex_destroy(&(unSemaforo->mutexValorSemaforo));
    free(unSemaforo->nombre);
    free(unSemaforo);
}

void sjf_destroy(t_pcb* pcb) {
    free(pcb->sjf);
    pcb_destroy(pcb);
}

void hrrn_destroy(t_pcb* pcb) {
    free(pcb->hrrn);
    pcb_destroy(pcb);
}
