#ifndef KERNEL_H_INCLUDED
#define KERNEL_H_INCLUDED

#include <sys/socket.h>

#include <commons/log.h>

#include "common_utils.h"
#include "connections.h"
#include "kernel_config.h"
#include "scheduler.h"
#include "stream.h"

#define KERNEL_LOG_DEST "bin/kernel.log"
#define KERNEL_MODULE_NAME "Kernel"

t_log *kernelLogger;
t_kernel_config *kernelCfg;

void aceptar_conexiones_kernel(int socketEscucha, struct sockaddr cliente, socklen_t len);
void crear_hilo_handler_conexion_entrante(int *socket);

#endif
