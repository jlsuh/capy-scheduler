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

typedef struct {
    double estActual;
} t_sjf;

typedef struct {
    time_t w;
    double s;
} t_hrrn;

bool pcb_is_hrrn(void);
bool pcb_is_sjf(void);
double response_ratio(t_pcb* pcb, time_t now);
t_deadlock* pcb_get_deadlock_info(t_pcb* self);
t_pcb* pcb_create(uint32_t* socket, uint32_t pid);
t_pcb* pcb_minimum_est(t_pcb* aPCB, t_pcb* anotherPCB);
t_status pcb_get_status(t_pcb* self);
uint32_t* pcb_get_socket(t_pcb* self);
uint32_t pcb_get_pid(t_pcb* self);
void pcb_algoritmo_destroy(t_pcb* self);
void pcb_algoritmo_init(t_pcb* self);
void pcb_algoritmo_update_next_est_info(t_pcb* self, time_t end, time_t start);
void pcb_destroy(t_pcb* self);
void pcb_set_status(t_pcb* self, t_status newStatus);

#endif
