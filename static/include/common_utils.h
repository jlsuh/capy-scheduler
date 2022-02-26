#ifndef COMMON_UTILS_H_INCLUDED
#define COMMON_UTILS_H_INCLUDED

#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <stdlib.h>

#include "kernel_structs.h"

///////////////////////////// COMMONS - Funciones Personalizadas /////////////////////////////
int list_get_index(t_list *list, bool (*cutting_condition)(void *, void *), void *target);
t_link_element *list_create_element(void *data);
t_link_element *list_find_element(t_list *self, bool (*cutting_condition)(t_link_element *, int));
void *list_find2(t_list *self, bool (*condition)(void *, void *), void *data);
void config_iterate_array(char **strings, void (*closure)(char *));
void intervalo_de_pausa(uint32_t duracionEnMilisegundos);

#endif
