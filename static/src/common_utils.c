#include "common_utils.h"

///////////////////////////// COMMONS - Funciones Personalizadas /////////////////////////////
void* list_find2(t_list* self, bool (*condition)(void*, void*), void* data) {
    t_link_element *element, *newElement = list_create_element(data);

    bool _find_by_condition(t_link_element * element, int i) {
        return element == NULL || condition(element->data, newElement->data);
    }
    element = list_find_element(self, _find_by_condition);

    free(newElement);
    return element != NULL ? element->data : NULL;
}

t_link_element* list_create_element(void* data) {
    t_link_element* element = malloc(sizeof(*element));
    element->data = data;
    element->next = NULL;
    return element;
}

t_link_element* list_find_element(t_list* self, bool (*cutting_condition)(t_link_element*, int)) {
    t_link_element* element = self->head;
    int index = 0;

    while (!cutting_condition(element, index)) {
        element = element->next;
        index++;
    }

    return element;
}

void config_iterate_array(char** strings, void (*closure)(char*)) {
    string_iterate_lines(strings, (void*)closure);
}

int list_get_index(t_list* list, bool (*cutting_condition)(void*, void*), void* target) {
    for (int i = 0; i < list_size(list); i++) {
        void* temp = list_get(list, i);
        if (cutting_condition(temp, target)) {
            return i;
        }
    }
    return -1;
}

void intervalo_de_pausa(uint32_t duracionEnMilisegundos) {
    const int SECS_MILISECS = 1000;        /* 1 seg == 1000 miliseg */
    const int MILISECS_NANOSECS = 1000000; /* 1 miliseg == 1000000 nanoseg */
    struct timespec timeSpec;
    timeSpec.tv_sec = duracionEnMilisegundos / SECS_MILISECS;
    timeSpec.tv_nsec = (duracionEnMilisegundos % SECS_MILISECS) * MILISECS_NANOSECS;
    nanosleep(&timeSpec, &timeSpec);
}
