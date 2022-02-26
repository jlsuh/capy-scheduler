#ifndef COMMONS_TEST_H_INCLUDED
#define COMMONS_TEST_H_INCLUDED

#include <CUnit/Basic.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdlib.h>

#include "common_utils.h"
#include "duplicated_logic_allocator.h"
#include "kernel_structs.h"

void test_commons_setup(void);
void test_commons_tear_down(void);

void test_dictionary(void);
void test_eliminar_un_elemento_de_la_lista_de_una_queue(void);
void test_es_posible_cambiar_de_string_en_un_instante_posterior(void);
void test_list_get_index(void);
void test_list_remove_retorna_nulo_en_caso_de_lista_vacia(void);

#endif
