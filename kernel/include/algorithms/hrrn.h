#ifndef HRRN_H_INCLUDED
#define HRRN_H_INCLUDED

#include <time.h>

typedef struct t_hrrn t_hrrn;

double hrrn_get_service_time(t_hrrn *);
t_hrrn *hrrn_create(void);
time_t hrrn_get_waiting_time(t_hrrn *);
void hrrn_set_service_time(t_hrrn *, double serviceTime);
void hrrn_set_waiting_time(t_hrrn *, time_t waitingTime);

#endif
