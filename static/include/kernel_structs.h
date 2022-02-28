#ifndef KERNEL_STRUCTS_H_INCLUDED
#define KERNEL_STRUCTS_H_INCLUDED

#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

typedef enum {
    OK_CONTINUE,
    OK_FINISH,
    MEMORIA,
    FAIL,
    DEADLOCK_END,
} t_handshake;

typedef enum {
    SEM_INIT,
    SEM_WAIT,
    SEM_POST,
    SEM_DESTROY,
    CALL_IO,
    MEM_ALLOC,
    MEM_FREE,
    MEM_READ,
    MEM_WRITE,
    MATE_CLOSE,
    MATE_INIT,
    SUSPEND_CARPINCHO
} t_tarea;

typedef struct {
    int32_t valor;
    char *nombre;
} t_tarea_sem;

typedef struct {
    char *nombre;
    void *mensaje;
} t_tarea_call_io;

typedef struct {
    t_tarea tipoTarea;
    void *contenidoTarea;
} t_peticion;

typedef struct {
    t_list *listaRecursos;
    pthread_mutex_t mutexRecursos;
} t_cola_recursos;

#endif
