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

typedef struct {
    t_queue *colaPCBs;
    char *nombre;
    int32_t valorInicial;
    int32_t valorActual;
    pthread_mutex_t mutexColaPCBs;
    pthread_mutex_t mutexValorSemaforo;
} t_recurso_sem;

typedef struct {
    t_queue *colaPCBs;
    char *nombre;
    uint32_t duracion;
    sem_t instanciasDisponibles;
    pthread_mutex_t mutexColaPCBs;
} t_recurso_io;

typedef struct {
    t_dictionary *semaforosQueRetiene;
    t_recurso_sem *esperaEnSemaforo;
    pthread_mutex_t mutexDict;
} t_deadlock;

typedef enum {
    NEW,
    READY,
    EXEC,
    BLOCKED,
    SUSBLOCKED,
    SUSREADY,
    EXIT
} t_status;

typedef struct {
    double estActual;
} t_sjf;

typedef struct {
    time_t w;
    double s;
} t_hrrn;

typedef struct t_pcb t_pcb;
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

typedef struct {
    t_list *lista;
    sem_t instanciasDisponibles;
    pthread_mutex_t mutex;
} t_cola_planificacion;

#endif
