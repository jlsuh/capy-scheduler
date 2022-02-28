#include "algorithms/hrrn.h"

#include <stdlib.h>

struct t_hrrn {
    time_t w;
    double s;
};

t_hrrn* hrrn_create(void) {
    t_hrrn* self = malloc(sizeof(*self));
    self->w = 0;
    self->s = 0;
    return self;
}

double hrrn_get_service_time(t_hrrn* self) {
    return self->s;
}

time_t hrrn_get_waiting_time(t_hrrn* self) {
    return self->w;
}

void hrrn_set_service_time(t_hrrn* self, double serviceTime) {
    self->s = serviceTime;
}

void hrrn_set_waiting_time(t_hrrn* self) {
    time(&(self->w));
}
