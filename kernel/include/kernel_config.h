#ifndef KERNEL_CONFIG_H_INCLUDED
#define KERNEL_CONFIG_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>

typedef struct {
    int32_t MEMORIA_SOCKET;
    char *IP_MEMORIA;
    char *PUERTO_MEMORIA;
    char *ALGORITMO_PLANIFICACION;
    double ESTIMACION_INICIAL;
    double ALFA;
    char **DISPOSITIVOS_IO;
    char **DURACIONES_IO;
    uint16_t GRADO_MULTIPROGRAMACION;
    uint16_t GRADO_MULTIPROCESAMIENTO;
    uint16_t TIEMPO_DEADLOCK;
} t_kernel_config;

extern t_log *kernelLogger;
extern t_kernel_config *kernelCfg;

t_kernel_config *kernel_cfg_create(void);
void kernel_config_initialize(void *kernelCfg, t_config *config);
void liberar_modulo_kernel(t_log *kernelLogger, t_kernel_config *kernelCfg);

#endif
