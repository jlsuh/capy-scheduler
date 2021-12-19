#include "kernel_config.h"

t_kernel_config* kernel_cfg_create(void) {
    t_kernel_config* newKernelCfg = malloc(sizeof(t_kernel_config));
    newKernelCfg->MEMORIA_SOCKET = 0;
    newKernelCfg->IP_MEMORIA = NULL;
    newKernelCfg->PUERTO_MEMORIA = NULL;
    newKernelCfg->ALGORITMO_PLANIFICACION = NULL;
    newKernelCfg->ESTIMACION_INICIAL = 0;
    newKernelCfg->ALFA = 0;
    newKernelCfg->DISPOSITIVOS_IO = NULL;
    newKernelCfg->DURACIONES_IO = NULL;
    newKernelCfg->GRADO_MULTIPROGRAMACION = 0;
    newKernelCfg->GRADO_MULTIPROCESAMIENTO = 0;
    newKernelCfg->TIEMPO_DEADLOCK = 0;
    return newKernelCfg;
}

void kernel_config_initialize(void* kernelCfg, t_config* config) {
    t_kernel_config* cfg = (t_kernel_config*) kernelCfg;
    cfg->IP_MEMORIA = strdup(config_get_string_value(config, "IP_MEMORIA"));
    cfg->PUERTO_MEMORIA = strdup(config_get_string_value(config, "PUERTO_MEMORIA"));
    cfg->ALGORITMO_PLANIFICACION = strdup(config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
    cfg->ESTIMACION_INICIAL = config_get_double_value(config, "ESTIMACION_INICIAL") / 1000;
    cfg->ALFA = config_get_double_value(config, "ALFA");
    cfg->DISPOSITIVOS_IO = config_get_array_value(config, "DISPOSITIVOS_IO");
    cfg->DURACIONES_IO = config_get_array_value(config, "DURACIONES_IO");
    cfg->GRADO_MULTIPROGRAMACION = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    cfg->GRADO_MULTIPROCESAMIENTO = config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");
    cfg->TIEMPO_DEADLOCK = config_get_int_value(config, "TIEMPO_DEADLOCK");
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
