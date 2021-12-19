#ifndef SCHEDULER_H_INCLUDED
#define SCHEDULER_H_INCLUDED

#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <commons/collections/list.h>

#include "buffer.h"
#include "common_utils.h"
#include "deadlock.h"
#include "kernel_config.h"
#include "kernel_structs.h"
#include "mem_adapter.h"
#include "stream.h"

bool algoritmo_hrrn_loaded(void);
bool algoritmo_sjf_loaded(void);
bool es_este_dispositivo_io(t_recurso_io *recursoIO, t_tarea_call_io *callIO);
bool existe_caso_de_suspension(void);
bool hay_procesos_en_bloqueados(void);
bool hay_procesos_en_esta_cola(t_cola_planificacion *cola);
bool hay_procesos_en_new(void);
bool hay_procesos_en_ready(void);
bool kernel_sem_wait(t_recurso_sem *sem, t_pcb *pcbWait);
bool realizar_tarea(t_buffer *buffer, uint32_t opCodeTarea, t_pcb *pcb);
double get_diferencial_de_tiempo(time_t tiempoFinal, time_t tiempoInicial);
double media_exponencial(double realAnterior, double estAnterior);
double response_ratio(t_pcb *pcb, time_t now);
int get_grado_multiprog_actual(void);
int pcb_get_posicion(t_pcb *pcb, t_list *lista);
t_cola_planificacion *cola_planificacion_create(int semInitVal);
t_cola_recursos *cola_recursos_create(void);
t_pcb *elegir_en_base_a_hrrn(t_cola_planificacion *colaPlanificacion);
t_pcb *elegir_en_base_a_sjf(t_cola_planificacion *colaPlanificacion);
t_pcb *elegir_pcb_segun_algoritmo(t_cola_planificacion *cola);
t_pcb *get_and_remove_primer_pcb_de_cola(t_cola_planificacion *cola);
t_pcb *kernel_sem_post(t_recurso_sem *sem, t_pcb *pcbPost);
t_pcb *pcb_create(uint32_t *socket, uint32_t pid);
t_pcb *pop_primer_pcb_de_cola_semaforo(t_recurso_sem *sem);
t_pcb *pop_ultimo_de_cola(t_cola_planificacion *cola);
t_pcb *sjf_pcb_menor_estimacion_entre(t_pcb *unPcb, t_pcb *otroPcb);
t_recurso_io *create_recurso_io(char *nombre, uint32_t duracion);
t_recurso_sem *recurso_sem_create(char *nombre, int32_t valor);
t_tarea_call_io *buffer_unpack_tarea_call_io(t_buffer *buffer);
t_tarea_sem *buffer_unpack_tarea_sem(t_buffer *buffer);
t_tarea_sem *buffer_unpack_tarea_sem_init(t_buffer *buffer);
uint32_t get_siguiente_pid();
void *encolar_en_new_nuevo_carpincho_entrante(void *socketHilo);
void *iniciar_corto_plazo(void *_);
void *iniciar_largo_plazo(void *_);
void *iniciar_mediano_plazo(void *_);
void *iniciar_rutina_deteccion_de_deadlocks(void *_);
void *iniciar_rutina_ejecucion_dispositivos_io(void *args);
void *liberar_carpinchos_en_exit(void *_);
void *transicion_susready_a_ready(void *_);
void agregar_pcb_a_cola(t_pcb *pcb, t_cola_planificacion *cola);
void atender_peticiones_del_carpincho(t_pcb *pcb);
void cambiar_estado_pcb(t_pcb *pcb, t_status nuevoEstado);
void ejecutar_rafagas_io(t_recurso_io *recursoIO, t_pcb *primerPCB);
void encolar_pcb_a_dispositivo_io(t_pcb *pcb, t_tarea_call_io *callIO);
void encolar_pcb_al_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void evaluar_suspension(void);
void hrrn_actualizar_info_para_siguiente_estimacion(t_pcb *pcb, time_t tiempoFinal, time_t tiempoInicial);
void hrrn_destroy(t_pcb *pcb);
void inicializar_dispositivos_io_config(t_cola_recursos *dispositivosIODelSistema);
void inicializar_hrrn(t_pcb *pcb);
void inicializar_sjf(t_pcb *pcb);
void iniciar_dispositivo_io(t_recurso_io *recursoIO);
void iniciar_ejecucion_lista_dispositivos_io(void);
void iniciar_planificacion(void);
void kernel_sem_init(t_tarea_sem *unaTareaDeSem, t_pcb *pcb);
void liberar_una_instancia_del_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void log_transition(const char *entityName, const char *prev, const char *post, int pid);
void pasar_de_blocked_a_ready(t_pcb *pcb);
void pasar_de_susblocked_a_susready(t_pcb *pcb);
void pcb_destroy(t_pcb *pcb);
void recurso_sem_destroy(t_recurso_sem *unSemaforo);
void remover_pcb_de_cola(t_pcb *pcb, t_cola_planificacion *cola);
void retener_una_instancia_del_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void sjf_actualizar_info_para_siguiente_estimacion(t_pcb *pcb, time_t tiempoFinal, time_t tiempoInicial);
void sjf_destroy(t_pcb *pcb);
void tarea_call_IO_destroy(t_tarea_call_io *unaTareaCallIO);
void tarea_sem_destroy(t_tarea_sem *unaTareaSem);

#endif
