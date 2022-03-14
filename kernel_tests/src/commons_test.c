#include "commons_test.h"

#include <CUnit/Basic.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdlib.h>

#include "common_utils.h"
#include "domain/pcb.h"

static t_list* lista;
static char* string;
static char* ALGORITHM = "SJF";

// @Before
void test_commons_setup(void) {
}

// @After
void test_commons_tear_down(void) {
}

void test_list_remove_retorna_nulo_en_caso_de_lista_vacia(void) {
    lista = list_create();
    t_pcb* pcb = NULL;

    if (!list_is_empty(lista)) {
        pcb = (t_pcb*)list_remove(lista, 0);
    }

    CU_ASSERT_PTR_NULL(pcb);

    list_destroy(lista);
}

void test_es_posible_cambiar_de_string_en_un_instante_posterior(void) {
    string = string_from_format("Susp/Ready");

    CU_ASSERT_STRING_EQUAL(string, "Susp/Ready");

    free(string);

    string = string_from_format("New");

    CU_ASSERT_STRING_EQUAL(string, "New");

    free(string);
}

void test_dictionary(void) {
    t_dictionary* dict = dictionary_create();
    t_dictionary* innerDict = dictionary_create();

    CU_ASSERT_TRUE(dictionary_is_empty(dict));
    CU_ASSERT_TRUE(dictionary_is_empty(innerDict));

    int data = 1;
    dictionary_put(innerDict, "SEM1", &data);
    dictionary_put(dict, "Carpincho1", innerDict);

    t_dictionary* sameInnerDict = dictionary_get(dict, "Carpincho1");
    int sameData = *(int*)dictionary_get(sameInnerDict, "SEM1");

    CU_ASSERT_EQUAL(sameData, 1);

    dictionary_destroy(sameInnerDict);
    dictionary_destroy(dict);
}

void test_list_get_index(void) {
    uint32_t* pid1 = malloc(sizeof(*pid1));
    uint32_t* pid2 = malloc(sizeof(*pid2));
    uint32_t* pid3 = malloc(sizeof(*pid3));

    *pid1 = 1;
    *pid2 = 2;
    *pid3 = 3;

    t_list* list = list_create();
    t_pcb* pcb1 = pcb_create(pid1, *pid1, ALGORITHM);
    t_pcb* pcb2 = pcb_create(pid2, *pid2, ALGORITHM);
    t_pcb* pcb3 = pcb_create(pid3, *pid3, ALGORITHM);

    list_add(list, pcb1);
    list_add(list, pcb3);
    list_add(list, pcb2);

    int index = list_get_index(list, __es_este_pcb, pid1);
    CU_ASSERT_EQUAL(index, 0);
    index = list_get_index(list, __es_este_pcb, pid2);
    CU_ASSERT_EQUAL(index, 2);
    index = list_get_index(list, __es_este_pcb, pid3);
    CU_ASSERT_EQUAL(index, 1);

    list_destroy(list);
    pcb_destroy(pcb1);
    pcb_destroy(pcb2);
    pcb_destroy(pcb3);
}
