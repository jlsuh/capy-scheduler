#include "domain/tarea_call_io.h"

#include <stdlib.h>

struct t_tarea_call_io {
    void *mensaje;
    char *nombre;
};

t_tarea_call_io *tarea_call_io_create(void) {
    t_tarea_call_io *self = malloc(sizeof(*self));
    self->mensaje = NULL;
    self->nombre = NULL;
    return self;
}

void tarea_call_IO_destroy(t_tarea_call_io *self) {
    free(self->mensaje);
    free(self->nombre);
    free(self);
}

char *tarea_call_io_get_nombre(t_tarea_call_io *self) {
    return self->nombre;
}

void *tarea_call_io_get_mensaje(t_tarea_call_io *self) {
    return self->mensaje;
}

void tarea_call_io_set_nombre(t_tarea_call_io *self, char *nombre) {
    self->nombre = nombre;
}

void tarea_call_io_set_mensaje(t_tarea_call_io *self, void *mensaje) {
    self->mensaje = mensaje;
}
