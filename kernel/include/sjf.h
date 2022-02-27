#ifndef SJF_H_INCLUDED
#define SJF_H_INCLUDED

typedef struct t_sjf t_sjf;

t_sjf* sjf_create(void);
void sjf_set_est_actual(t_sjf*, double estActual);
double sjf_get_est_actual(t_sjf*);

#endif
