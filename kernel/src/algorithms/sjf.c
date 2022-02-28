#include "algorithms/sjf.h"

#include <stdlib.h>

struct t_sjf {
    double estActual;
};

t_sjf* sjf_create(void) {
    t_sjf* self = malloc(sizeof(*self));
    self->estActual = 0;
    return self;
}

double sjf_get_est_actual(t_sjf* self) {
    return self->estActual;
}

void sjf_set_est_actual(t_sjf* self, double estActual) {
    self->estActual = estActual;
}
