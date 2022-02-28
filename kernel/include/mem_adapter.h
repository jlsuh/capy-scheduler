#ifndef MEM_ADAPTER_H_INCLUDED
#define MEM_ADAPTER_H_INCLUDED

#include "buffer.h"
#include "domain/pcb.h"

void enviar_mate_close_a_memoria(t_pcb* pcb);
void enviar_memalloc_a_memoria(t_pcb* pcb, t_buffer* buffer);
void enviar_memfree_a_memoria(t_pcb* pcb, t_buffer* buffer);
void enviar_memread_a_memoria(t_pcb* pcb, t_buffer* buffer);
void enviar_memwrite_a_memoria(t_pcb* pcb, t_buffer* buffer);
void enviar_suspension_de_carpincho_a_memoria(t_pcb* pcb);

#endif
