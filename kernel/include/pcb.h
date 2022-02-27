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

t_deadlock* pcb_get_deadlock_info(t_pcb*);
uint32_t pcb_get_pid(t_pcb*);
t_status pcb_get_status(t_pcb*);
uint32_t* pcb_get_socket(t_pcb*);
void pcb_algoritmo_destroy(t_pcb*);
void pcb_algoritmo_init(t_pcb*);
void pcb_algoritmo_update_next_est_info(t_pcb*, time_t end, time_t start);
void pcb_set_status(t_pcb*, t_status);
t_sjf* pcb_get_sjf(t_pcb*);
void pcb_set_sjf(t_pcb*, t_sjf*);
void pcb_set_est_actual(t_pcb*, double newEstActual);
void pcb_sjf_create(t_pcb*);
void pcb_hrrn_create(t_pcb*);
void pcb_set_service_time(t_pcb*, double newServiceTime);
double pcb_get_service_time(t_pcb*);
void pcb_set_waiting_time(t_pcb*);
void pcb_set_waiting_time(t_pcb*);
t_pcb* pcb_create(uint32_t* socket, uint32_t pid);
double pcb_get_waiting_time(t_pcb*);
bool pcb_is_sjf(void);
bool pcb_is_hrrn(void);
t_pcb* pcb_minimum_est(t_pcb*, t_pcb*);

#endif
