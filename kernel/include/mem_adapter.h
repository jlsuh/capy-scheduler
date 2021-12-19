#ifndef MEM_ADAPTER_H_INCLUDED
#define MEM_ADAPTER_H_INCLUDED

#include <stdio.h>

#include <commons/log.h>

#include "buffer.h"
#include "stream.h"
#include "kernel_config.h"
#include "kernel_structs.h"

#define EXEC_MODE "DEV_MODE"

extern t_log *kernelLogger;
extern t_kernel_config *kernelCfg;
extern pthread_mutex_t mutexMemSocket;

bool es_deploy_mode(void);
void enviar_mate_close_a_memoria(t_pcb *pcb);
void enviar_memalloc_a_memoria(t_pcb *pcb, t_buffer *buffer);
void enviar_memfree_a_memoria(t_pcb *pcb, t_buffer *buffer);
void enviar_memread_a_memoria(t_pcb *pcb, t_buffer *buffer);
void enviar_memwrite_a_memoria(t_pcb *pcb, t_buffer *buffer);
void enviar_suspension_de_carpincho_a_memoria(t_pcb *pcb);

#endif