#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED

#include "kernel_structs.h"

typedef struct t_pcb t_pcb;

typedef enum {
    NEW,
    READY,
    EXEC,
    BLOCKED,
    SUSBLOCKED,
    SUSREADY,
    EXIT
} t_status;

bool pcb_is_hrrn(void);
bool pcb_is_sjf(void);
double response_ratio(t_pcb *, time_t now);
t_deadlock *pcb_get_deadlock_info(t_pcb *);
t_pcb *pcb_create(uint32_t *socket, uint32_t pid);
t_pcb *pcb_minimum_est(t_pcb *, t_pcb *);
t_status pcb_get_status(t_pcb *);
uint32_t *pcb_get_socket(t_pcb *);
uint32_t pcb_get_pid(t_pcb *);
void pcb_algoritmo_destroy(t_pcb *);
void pcb_algoritmo_init(t_pcb *);
void pcb_algoritmo_update_next_est_info(t_pcb *, time_t end, time_t start);
void pcb_destroy(t_pcb *);
void pcb_set_status(t_pcb *, t_status newStatus);

#endif
