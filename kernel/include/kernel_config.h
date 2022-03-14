#ifndef KERNEL_CONFIG_H_INCLUDED
#define KERNEL_CONFIG_H_INCLUDED

#include <commons/log.h>
#include <stdint.h>

typedef struct t_kernel_config t_kernel_config;

char *kernel_config_get_mem_ip(t_kernel_config *);
char *kernel_config_get_mem_port(t_kernel_config *);
char **kernel_config_get_dispositivosIO(t_kernel_config *);
char **kernel_config_get_duracionesIO(t_kernel_config *);
char const *kernel_config_get_algoritmo_planificacion(t_kernel_config *);
double kernel_config_get_alfa(t_kernel_config *);
double kernel_config_get_est_inicial(t_kernel_config *);
int32_t kernel_config_get_mem_socket(t_kernel_config *);
t_kernel_config *kernel_config_create(char *kernelConfigPath);
uint16_t kernel_config_get_grado_multiproc(t_kernel_config *);
uint16_t kernel_config_get_grado_multiprog(t_kernel_config *);
uint16_t kernel_config_get_tiempo_deadlock(t_kernel_config *);
void kernel_config_set_mem_socket(t_kernel_config *, int32_t socket);
void liberar_modulo_kernel(t_log *, t_kernel_config *);

#endif
