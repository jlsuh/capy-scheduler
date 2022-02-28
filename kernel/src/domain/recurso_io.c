#include "domain/recurso_io.h"

#include <stdlib.h>

struct t_recurso_io {
    t_queue* colaPCBs;
    char* nombre;
    uint32_t duracion;
    sem_t* instanciasDisponibles;
    pthread_mutex_t* mutexColaPCBs;
};

t_recurso_io* recurso_io_create(char* nombre, uint32_t duracion) {
    t_recurso_io* self = malloc(sizeof(*self));
    self->colaPCBs = queue_create();
    self->nombre = nombre;
    self->duracion = duracion;
    self->instanciasDisponibles = malloc(sizeof(*self->instanciasDisponibles));
    self->mutexColaPCBs = malloc(sizeof(*self->mutexColaPCBs));
    sem_init(self->instanciasDisponibles, 0, 0);
    pthread_mutex_init(self->mutexColaPCBs, NULL);
    return self;
}

char* recurso_io_get_nombre(t_recurso_io* self) {
    return self->nombre;
}

pthread_mutex_t* recurso_io_get_mutex_cola_pcbs(t_recurso_io* self) {
    return self->mutexColaPCBs;
}

t_queue* recurso_io_get_cola_pcbs(t_recurso_io* self) {
    return self->colaPCBs;
}

sem_t* recurso_io_get_sem_instancias_disponibles(t_recurso_io* self) {
    return self->instanciasDisponibles;
}

uint32_t recurso_io_get_duracion(t_recurso_io* self) {
    return self->duracion;
}
