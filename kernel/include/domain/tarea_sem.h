#ifndef TAREA_SEM_H_INCLUDED
#define TAREA_SEM_H_INCLUDED

#include <stdint.h>

typedef struct t_tarea_sem t_tarea_sem;

char *tarea_sem_get_nombre(t_tarea_sem *);
int32_t tarea_sem_get_valor_inicial(t_tarea_sem *);
t_tarea_sem *tarea_sem_create(void);
void tarea_sem_destroy(t_tarea_sem *unaTareaSem);
void tarea_sem_set_nombre(t_tarea_sem *, char *nombre);
void tarea_sem_set_valor_inicial(t_tarea_sem *, int32_t valorInicial);

#endif
