#ifndef DUPLICATED_LOGIC_ALLOCATOR_H_INCLUDED
#define DUPLICATED_LOGIC_ALLOCATOR_H_INCLUDED

#include <commons/collections/list.h>
#include <stdbool.h>

bool __deadlock_eliminar_pcb_de_lista(t_pcb *pcb, t_list *lista);
bool es_este_pcb(void *pcbVoid, void *pidVoid);
bool kernel_sem_wait(t_recurso_sem *sem, t_pcb *pcbWait);
double get_diferencial_de_tiempo(clock_t tiempoFinal, clock_t tiempoInicial);
double __media_exponencial(double realAnterior, double estAnterior);
double pcb_response_ratio(t_pcb *pcb, clock_t now);
t_cola_recursos *cola_recursos_create(void);
t_pcb *elegir_en_base_a_hrrn(t_list *lista);
t_pcb *elegir_en_base_a_sjf(t_list *lista);
t_pcb *kernel_sem_post(t_recurso_sem *sem, t_pcb *pcbPost);
t_pcb *pcb_create(uint32_t *pid, const char *algoritmo);
t_pcb *sjf_pcb_menor_estimacion_entre(t_pcb *unPcb, t_pcb *otroPcb);
t_recurso_sem *recurso_sem_create(char *nombre, int32_t valor);
void *pcb_maximum_pid(void *pcbVoid1, void *pcbVoid2);
void cola_recursos_destroy(t_cola_recursos *colaRecursos);
void hrrn_actualizar_info_para_siguiente_estimacion(t_pcb *pcb, clock_t tiempoFinal, clock_t tiempoInicial);
void hrrn_destroy(t_pcb *pcb);
void inicializar_hrrn(t_pcb *pcb);
void inicializar_sjf(t_pcb *pcb);
void liberar_una_instancia_del_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void pcb_destroy(t_pcb *pcb);
void recurso_sem_destroy(t_recurso_sem *unSemaforo);
void retener_una_instancia_del_semaforo(t_pcb *pcb, t_recurso_sem *sem);
void sjf_actualizar_estimacion_actual(t_pcb *pcb, double realAnterior, double estAnterior);
void sjf_actualizar_info_para_siguiente_estimacion(t_pcb *pcb, clock_t tiempoFinal, clock_t tiempoInicial);
void sjf_destroy(t_pcb *pcb);

#endif
