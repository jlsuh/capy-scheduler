#include "domain/cola_recursos.h"

#include <stdlib.h>

struct t_cola_recursos {
    t_list* listaRecursos;
    pthread_mutex_t* mutexRecursos;
};

t_cola_recursos* cola_recursos_create(void) {
    t_cola_recursos* colaRecursos = malloc(sizeof(*colaRecursos));
    colaRecursos->listaRecursos = list_create();
    colaRecursos->mutexRecursos = malloc(sizeof(*colaRecursos->mutexRecursos));
    pthread_mutex_init(colaRecursos->mutexRecursos, NULL);
    return colaRecursos;
}

t_list* cola_recursos_get_list(t_cola_recursos* self) {
    return self->listaRecursos;
}

pthread_mutex_t* cola_recursos_get_mutex(t_cola_recursos* self) {
    return self->mutexRecursos;
}
