#include "domain/cola_planificacion.h"

#include <stdlib.h>

struct t_cola_planificacion {
    t_list* lista;
    sem_t* instanciasDisponibles;
    pthread_mutex_t* mutex;
};

t_cola_planificacion* cola_planificacion_create(int semInitVal) {
    t_cola_planificacion* self = malloc(sizeof(*self));
    self->lista = list_create();
    self->mutex = malloc(sizeof(*self->mutex));
    self->instanciasDisponibles = malloc(sizeof(*self->instanciasDisponibles));
    pthread_mutex_init(self->mutex, NULL);
    sem_init(self->instanciasDisponibles, 0, semInitVal);
    return self;
}

void cola_planificacion_destroy(t_cola_planificacion* self) {
    list_destroy(self->lista);
    pthread_mutex_destroy(self->mutex);
    sem_destroy(self->instanciasDisponibles);
    free(self->mutex);
    free(self->instanciasDisponibles);
    free(self);
}

t_list* cola_planificacion_get_list(t_cola_planificacion* self) {
    return self->lista;
}

sem_t* cola_planificacion_get_instancias_disponibles(t_cola_planificacion* self) {
    return self->instanciasDisponibles;
}

pthread_mutex_t* cola_planificacion_get_mutex(t_cola_planificacion* self) {
    return self->mutex;
}
