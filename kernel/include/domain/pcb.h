#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED

#include "deadlock.h"

typedef struct t_pcb t_pcb;
typedef enum t_status t_status;

bool pcb_espera_algun_semaforo(void* pcbVoid);
bool pcb_is_hrrn(void);
bool pcb_is_sjf(void);
bool pcb_status_is_blocked(t_pcb*);
bool pcb_status_is_susblocked(t_pcb*);
double pcb_response_ratio(t_pcb* pcb, time_t now);
t_deadlock* pcb_get_deadlock_info(t_pcb*);
t_pcb* pcb_create(uint32_t* socket, uint32_t pid);
t_pcb* pcb_minimum_est(t_pcb*, t_pcb*);
t_status pcb_get_status(t_pcb*);
uint32_t* pcb_get_socket(t_pcb*);
uint32_t pcb_get_pid(t_pcb*);
void* pcb_maximum_pid(void* aPCBVoid, void* anotherPCBVoid);
void pcb_algoritmo_destroy(t_pcb*);
void pcb_algoritmo_init(t_pcb*);
void pcb_algoritmo_update_next_est_info(t_pcb*, time_t end, time_t start);
void pcb_destroy(t_pcb*);
void pcb_set_status(t_pcb*, t_status newStatus);
void pcb_transition_to_blocked(t_pcb*);
void pcb_transition_to_exec(t_pcb*);
void pcb_transition_to_exit(t_pcb*);
void pcb_transition_to_ready(t_pcb*);
void pcb_transition_to_susblocked(t_pcb*);
void pcb_transition_to_susready(t_pcb*);

#endif
