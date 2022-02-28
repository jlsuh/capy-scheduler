#include "domain/tarea_sem.h"

#include <stdlib.h>

struct t_tarea_sem {
    int32_t valorInicial;
    char* nombre;
};

void tarea_sem_destroy(t_tarea_sem* unaTareaSem) {
    free(unaTareaSem->nombre);
    free(unaTareaSem);
}

t_tarea_sem* tarea_sem_create(void) {
    t_tarea_sem* self = malloc(sizeof(*self));
    self->nombre = NULL;
    self->valorInicial = -1;
    return self;
}

char* tarea_sem_get_nombre(t_tarea_sem* self) {
    return self->nombre;
}

int32_t tarea_sem_get_valor_inicial(t_tarea_sem* self) {
    return self->valorInicial;
}

void tarea_sem_set_nombre(t_tarea_sem* self, char* nombre) {
    self->nombre = nombre;
}

void tarea_sem_set_valor_inicial(t_tarea_sem* self, int32_t valorInicial) {
    self->valorInicial = valorInicial;
}
