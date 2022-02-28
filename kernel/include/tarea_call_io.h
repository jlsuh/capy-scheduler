#ifndef TAREA_CALL_IO_H_INCLUDED
#define TAREA_CALL_IO_H_INCLUDED

typedef struct t_tarea_call_io t_tarea_call_io;

t_tarea_call_io *tarea_call_io_create(void);
void tarea_call_IO_destroy(t_tarea_call_io *);
char *tarea_call_io_get_nombre(t_tarea_call_io *);
void *tarea_call_io_get_mensaje(t_tarea_call_io *);
void tarea_call_io_set_nombre(t_tarea_call_io *, char *nombre);
void tarea_call_io_set_mensaje(t_tarea_call_io *, void *mensaje);

#endif
