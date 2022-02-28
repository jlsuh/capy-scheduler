#include "domain/pcb.h"

#include <commons/collections/dictionary.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "algorithms/hrrn.h"
#include "algorithms/sjf.h"
#include "kernel_config.h"

struct t_pcb {
    uint32_t *socket;
    uint32_t pid;
    t_status status;
    t_sjf *sjf;
    t_hrrn *hrrn;
    void (*algoritmo_init)(t_pcb *self);
    void (*algoritmo_destroy)(t_pcb *self);
    void (*algoritmo_update_next_est_info)(t_pcb *self, time_t tiempoFinal, time_t tiempoInicial);
    t_deadlock *deadlockInfo;
};

extern t_kernel_config *kernelCfg;

static double __time_differential(time_t tiempoFinal, time_t tiempoInicial) {
    double diferencialT = difftime(tiempoFinal, tiempoInicial);
    return diferencialT;
}

static void __pcb_hrrn_create(t_pcb *self) {
    self->hrrn = hrrn_create();
    double serviceTime = kernel_config_get_est_inicial(kernelCfg);
    hrrn_set_service_time(self->hrrn, serviceTime);
    hrrn_set_waiting_time(self->hrrn);
}

static double __media_exponencial(double realAnterior, double estAnterior) {
    /* Est(n) = α . R(n-1) + (1 - α) . Est(n-1) */
    double const alfa = kernel_config_get_alfa(kernelCfg);
    return alfa * realAnterior + (1 - alfa) * estAnterior;
}

static void __pcb_sjf_create(t_pcb *self) {
    self->sjf = sjf_create();
    double initialEst = kernel_config_get_est_inicial(kernelCfg);
    sjf_set_est_actual(self->sjf, initialEst);
}

static void __pcb_hrrn_destroy(t_pcb *pcb) {
    free(pcb->hrrn);
    pcb_destroy(pcb);
}

static void __pcb_sjf_destroy(t_pcb *pcb) {
    free(pcb->sjf);
    pcb_destroy(pcb);
}

static void __pcb_hrrn_est_update(t_pcb *self, time_t tiempoFinal, time_t tiempoInicial) {
    double realAnterior = __time_differential(tiempoFinal, tiempoInicial);
    double newServiceTime = __media_exponencial(realAnterior, hrrn_get_service_time(self->hrrn));
    hrrn_set_service_time(self->hrrn, newServiceTime);
    hrrn_set_waiting_time(self->hrrn);
}

static void __pcb_sjf_est_update(t_pcb *self, time_t tiempoFinal, time_t tiempoInicial) {
    double realAnterior = __time_differential(tiempoFinal, tiempoInicial);
    double newEstActual = __media_exponencial(realAnterior, sjf_get_est_actual(self->sjf));
    sjf_set_est_actual(self->sjf, newEstActual);
}

t_pcb *pcb_create(uint32_t *socket, uint32_t pid) {
    t_pcb *self = malloc(sizeof(*self));
    self->socket = socket;
    self->pid = pid;
    self->status = NEW;
    self->sjf = NULL;
    self->hrrn = NULL;
    if (pcb_is_sjf()) {
        self->algoritmo_init = __pcb_sjf_create;
        self->algoritmo_destroy = __pcb_sjf_destroy;
        self->algoritmo_update_next_est_info = __pcb_sjf_est_update;
    } else if (pcb_is_hrrn()) {
        self->algoritmo_init = __pcb_hrrn_create;
        self->algoritmo_destroy = __pcb_hrrn_destroy;
        self->algoritmo_update_next_est_info = __pcb_hrrn_est_update;
    }
    self->deadlockInfo = deadlock_create();
    return self;
}

void pcb_destroy(t_pcb *self) {
    free(self->socket);
    deadlock_destroy(self->deadlockInfo);
    free(self);
}

void *pcb_maximum_pid(void *aPCBVoid, void *anotherPCBVoid) {
    t_pcb *aPCB = aPCBVoid;
    t_pcb *anotherPCB = anotherPCBVoid;
    return pcb_get_pid(aPCB) > pcb_get_pid(anotherPCB) ? aPCB : anotherPCB;
}

t_pcb *pcb_minimum_est(t_pcb *aPCB, t_pcb *anotherPCB) {
    return sjf_get_est_actual(aPCB->sjf) <= sjf_get_est_actual(anotherPCB->sjf) ? aPCB : anotherPCB;
}

bool pcb_is_hrrn(void) {
    return strcmp(kernel_config_get_algoritmo_planificacion(kernelCfg), "HRRN") == 0;
}

bool pcb_is_sjf(void) {
    return strcmp(kernel_config_get_algoritmo_planificacion(kernelCfg), "SJF") == 0;
}

bool pcb_espera_algun_semaforo(void *pcbVoid) {
    t_pcb *pcb = (t_pcb *)pcbVoid;
    return deadlock_espera_en_semaforo(pcb->deadlockInfo);
}

double response_ratio(t_pcb *pcb, time_t now) {
    return __time_differential(now, hrrn_get_waiting_time(pcb->hrrn)) / hrrn_get_service_time(pcb->hrrn) + 1;
}

uint32_t *pcb_get_socket(t_pcb *self) {
    return self->socket;
}

uint32_t pcb_get_pid(t_pcb *self) {
    return self->pid;
}

t_status pcb_get_status(t_pcb *self) {
    return self->status;
}

t_deadlock *pcb_get_deadlock_info(t_pcb *self) {
    return self->deadlockInfo;
}

void pcb_set_status(t_pcb *self, t_status newStatus) {
    self->status = newStatus;
}

void pcb_algoritmo_destroy(t_pcb *self) {
    self->algoritmo_destroy(self);
}

void pcb_algoritmo_init(t_pcb *self) {
    self->algoritmo_init(self);
}

void pcb_algoritmo_update_next_est_info(t_pcb *self, time_t end, time_t start) {
    self->algoritmo_update_next_est_info(self, end, start);
}
