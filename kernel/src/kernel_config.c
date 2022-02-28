#include "kernel_config.h"

#include <commons/config.h>
#include <commons/string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "module_config.h"

#define KERNEL_CONFIG_PATH "cfg/kernel_config.cfg"

extern t_log* kernelLogger;
extern t_kernel_config* kernelCfg;

struct t_kernel_config {
    int32_t MEMORIA_SOCKET;
    char* IP_MEMORIA;
    char* PUERTO_MEMORIA;
    char* ALGORITMO_PLANIFICACION;
    double ESTIMACION_INICIAL;
    double ALFA;
    char** DISPOSITIVOS_IO;
    char** DURACIONES_IO;
    uint16_t GRADO_MULTIPROGRAMACION;
    uint16_t GRADO_MULTIPROCESAMIENTO;
    uint16_t TIEMPO_DEADLOCK;
};

static void __kernel_config_initializer(void* moduleConfig, t_config* tempCfg) {
    t_kernel_config* kernelConfig = (t_kernel_config*)moduleConfig;
    const int MILISECS_TO_SECS = 1000;
    kernelConfig->IP_MEMORIA = strdup(config_get_string_value(tempCfg, "IP_MEMORIA"));
    kernelConfig->PUERTO_MEMORIA = strdup(config_get_string_value(tempCfg, "PUERTO_MEMORIA"));
    kernelConfig->ALGORITMO_PLANIFICACION = strdup(config_get_string_value(tempCfg, "ALGORITMO_PLANIFICACION"));
    kernelConfig->ESTIMACION_INICIAL = config_get_double_value(tempCfg, "ESTIMACION_INICIAL") / MILISECS_TO_SECS;
    kernelConfig->ALFA = config_get_double_value(tempCfg, "ALFA");
    kernelConfig->DISPOSITIVOS_IO = config_get_array_value(tempCfg, "DISPOSITIVOS_IO");
    kernelConfig->DURACIONES_IO = config_get_array_value(tempCfg, "DURACIONES_IO");
    kernelConfig->GRADO_MULTIPROGRAMACION = config_get_int_value(tempCfg, "GRADO_MULTIPROGRAMACION");
    kernelConfig->GRADO_MULTIPROCESAMIENTO = config_get_int_value(tempCfg, "GRADO_MULTIPROCESAMIENTO");
    kernelConfig->TIEMPO_DEADLOCK = config_get_int_value(tempCfg, "TIEMPO_DEADLOCK");
    kernelConfig->MEMORIA_SOCKET = 0;
}

t_kernel_config* kernel_config_create(void) {
    t_kernel_config* self = malloc(sizeof(*self));
    config_init(self, KERNEL_CONFIG_PATH, kernelLogger, __kernel_config_initializer);
    return self;
}

void kernel_config_set_mem_socket(t_kernel_config* self, int32_t socket) {
    self->MEMORIA_SOCKET = socket;
}

char* kernel_config_get_mem_ip(t_kernel_config* self) {
    return self->IP_MEMORIA;
}

char* kernel_config_get_mem_port(t_kernel_config* self) {
    return self->PUERTO_MEMORIA;
}

char const* kernel_config_get_algoritmo_planificacion(t_kernel_config* self) {
    return self->ALGORITMO_PLANIFICACION;
}

char** kernel_config_get_duracionesIO(t_kernel_config* self) {
    return self->DURACIONES_IO;
}

char** kernel_config_get_dispositivosIO(t_kernel_config* self) {
    return self->DISPOSITIVOS_IO;
}

double kernel_config_get_est_inicial(t_kernel_config* self) {
    return self->ESTIMACION_INICIAL;
}

double kernel_config_get_alfa(t_kernel_config* self) {
    return self->ALFA;
}

int32_t kernel_config_get_mem_socket(t_kernel_config* self) {
    return self->MEMORIA_SOCKET;
}

uint16_t kernel_config_get_grado_multiprog(t_kernel_config* self) {
    return self->GRADO_MULTIPROGRAMACION;
}

uint16_t kernel_config_get_grado_multiproc(t_kernel_config* self) {
    return self->GRADO_MULTIPROCESAMIENTO;
}

uint16_t kernel_config_get_tiempo_deadlock(t_kernel_config* self) {
    return self->TIEMPO_DEADLOCK;
}

void liberar_modulo_kernel(t_log* kernelLogger, t_kernel_config* kernelCfg) {
    log_destroy(kernelLogger);
    close(kernelCfg->MEMORIA_SOCKET);
    free(kernelCfg->IP_MEMORIA);
    free(kernelCfg->PUERTO_MEMORIA);
    free(kernelCfg->ALGORITMO_PLANIFICACION);
    string_array_destroy(kernelCfg->DISPOSITIVOS_IO);
    string_array_destroy(kernelCfg->DURACIONES_IO);
    free(kernelCfg);
}
