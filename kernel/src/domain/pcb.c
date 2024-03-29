#include "domain/pcb.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "algorithms/sjf.h"
#include "kernel_config.h"

enum t_status {
    NEW,
    READY,
    EXEC,
    BLOCKED,
    SUSBLOCKED,
    SUSREADY,
    EXIT
};

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

static void __pcb_hrrn_create(t_pcb *self) {
    self->hrrn = hrrn_create();
    double serviceTime = kernel_config_get_est_inicial(kernelCfg);
    hrrn_set_service_time(self->hrrn, serviceTime);
    time_t curr;
    hrrn_set_waiting_time(self->hrrn, time(&curr));
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

static double __time_differential(time_t tiempoFinal, time_t tiempoInicial) {
    double diferencialT = difftime(tiempoFinal, tiempoInicial);
    return diferencialT;
}

static double __media_exponencial(double realAnterior, double estAnterior) {
    /* Est(n) = α . R(n-1) + (1 - α) . Est(n-1) */
    double const alfa = kernel_config_get_alfa(kernelCfg);
    return alfa * realAnterior + (1 - alfa) * estAnterior;
}

static void __pcb_hrrn_est_update(t_pcb *self, time_t tiempoFinal, time_t tiempoInicial) {
    double realAnterior = __time_differential(tiempoFinal, tiempoInicial);
    double newServiceTime = __media_exponencial(realAnterior, hrrn_get_service_time(self->hrrn));
    hrrn_set_service_time(self->hrrn, newServiceTime);
    time_t curr;
    hrrn_set_waiting_time(self->hrrn, time(&curr));
}

static void __pcb_sjf_est_update(t_pcb *self, time_t tiempoFinal, time_t tiempoInicial) {
    double realAnterior = __time_differential(tiempoFinal, tiempoInicial);
    double newEstActual = __media_exponencial(realAnterior, sjf_get_est_actual(self->sjf));
    sjf_set_est_actual(self->sjf, newEstActual);
}

bool __es_este_pcb(void *pcbVoid, void *pidVoid) {
    t_pcb *pcb = (t_pcb *)pcbVoid;
    uint32_t pid = *(uint32_t *)pidVoid;
    return pcb_get_pid(pcb) == pid;
}

bool pcb_status_is_blocked(t_pcb *self) {
    return BLOCKED == self->status;
}

bool pcb_status_is_susblocked(t_pcb *self) {
    return SUSBLOCKED == self->status;
}

bool pcb_is_hrrn(char *algorithm) {
    return strcmp(algorithm, "HRRN") == 0;
}

bool pcb_is_sjf(char *algorithm) {
    return strcmp(algorithm, "SJF") == 0;
}

t_pcb *pcb_create(uint32_t *socket, uint32_t pid, char *algorithm) {
    t_pcb *self = malloc(sizeof(*self));
    self->socket = socket;
    self->pid = pid;
    self->status = NEW;
    self->sjf = NULL;
    self->hrrn = NULL;
    if (pcb_is_sjf(algorithm)) {
        self->algoritmo_init = __pcb_sjf_create;
        self->algoritmo_destroy = __pcb_sjf_destroy;
        self->algoritmo_update_next_est_info = __pcb_sjf_est_update;
    } else if (pcb_is_hrrn(algorithm)) {
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

bool pcb_espera_algun_semaforo(void *pcbVoid) {
    t_pcb *pcb = (t_pcb *)pcbVoid;
    return deadlock_espera_en_semaforo(pcb->deadlockInfo);
}

double pcb_response_ratio(t_pcb *pcb, time_t now) {
    return __time_differential(now, hrrn_get_waiting_time(pcb->hrrn)) / hrrn_get_service_time(pcb->hrrn) + 1;
}

void pcb_transition_to_ready(t_pcb *self) {
    self->status = READY;
}

void pcb_transition_to_susready(t_pcb *self) {
    self->status = SUSREADY;
}

void pcb_transition_to_blocked(t_pcb *self) {
    self->status = BLOCKED;
}

void pcb_transition_to_susblocked(t_pcb *self) {
    self->status = SUSBLOCKED;
}

void pcb_transition_to_exit(t_pcb *self) {
    self->status = EXIT;
}

void pcb_transition_to_exec(t_pcb *self) {
    self->status = EXEC;
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

double pcb_get_est_actual(t_pcb *self) {
    return sjf_get_est_actual(self->sjf);
}

double pcb_get_service_time(t_pcb *self) {
    return hrrn_get_service_time(self->hrrn);
}

time_t pcb_get_waiting_time(t_pcb *self) {
    return hrrn_get_waiting_time(self->hrrn);
}

t_hrrn *pcb_get_hrnn(t_pcb *self) {
    return self->hrrn;
}

void pcb_set_status(t_pcb *self, t_status newStatus) {
    self->status = newStatus;
}

void pcb_set_service_time(t_pcb *self, double serviceTime) {
    hrrn_set_service_time(self->hrrn, serviceTime);
}

void pcb_set_waiting_time(t_pcb *self, time_t waitingTime) {
    hrrn_set_waiting_time(self->hrrn, waitingTime);
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
