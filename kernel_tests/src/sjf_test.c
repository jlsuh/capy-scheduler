#include "sjf_test.h"

#include <CUnit/Basic.h>
#include <stdlib.h>

#include "duplicated_logic_allocator.h"
#include "kernel_structs.h"

extern double ESTIMACION_INICIAL;
extern double ALFA;
static t_list* pcbsReady;
static t_pcb* carpincho1;
static t_pcb* carpincho2;
static t_pcb* carpincho3;
static t_pcb* carpincho4;
static t_pcb* carpincho5;
static uint32_t* ID1;
static uint32_t* ID2;
static uint32_t* ID3;
static uint32_t* ID4;
static uint32_t* ID5;

// @Before
void test_sjf_setup(void) {
    ESTIMACION_INICIAL = 3;
    ALFA = 0.5;

    ID1 = malloc(sizeof(*ID1));
    ID2 = malloc(sizeof(*ID2));
    ID3 = malloc(sizeof(*ID3));
    ID4 = malloc(sizeof(*ID4));
    ID5 = malloc(sizeof(*ID5));

    *ID1 = 1;
    *ID2 = 2;
    *ID3 = 3;
    *ID4 = 4;
    *ID5 = 5;

    pcbsReady = list_create();
    carpincho1 = pcb_create(ID1, "SJF");
    carpincho2 = pcb_create(ID2, "SJF");
    carpincho3 = pcb_create(ID3, "SJF");
    carpincho4 = pcb_create(ID4, "SJF");
    carpincho5 = pcb_create(ID5, "SJF");

    carpincho1->algoritmo_init(carpincho1);
    carpincho2->algoritmo_init(carpincho2);
    carpincho3->algoritmo_init(carpincho3);
    carpincho4->algoritmo_init(carpincho4);
    carpincho5->algoritmo_init(carpincho5);
}

// @After
void test_sjf_tear_down(void) {
    // list_destroy_and_destroy_elements(pcbsReady, pcb_destroy);
    carpincho1->algoritmo_destroy(carpincho1);
    carpincho2->algoritmo_destroy(carpincho2);
    carpincho3->algoritmo_destroy(carpincho3);
    carpincho4->algoritmo_destroy(carpincho4);
    carpincho5->algoritmo_destroy(carpincho5);
    list_destroy(pcbsReady);
}

void test_la_estimacion_actual_inicial_es_la_misma_para_todos(void) {
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho5);
    list_add(pcbsReady, carpincho4);

    CU_ASSERT_EQUAL(carpincho1->sjf->estActual, carpincho2->sjf->estActual);
    CU_ASSERT_EQUAL(carpincho2->sjf->estActual, carpincho3->sjf->estActual);
    CU_ASSERT_EQUAL(carpincho3->sjf->estActual, carpincho4->sjf->estActual);
    CU_ASSERT_EQUAL(carpincho4->sjf->estActual, carpincho5->sjf->estActual);
}

void test_se_elige_por_fifo_en_caso_de_empate_1(void) {
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho1);

    t_pcb* pcbConMenorEst = elegir_en_base_a_sjf(pcbsReady);

    CU_ASSERT_EQUAL(pcbConMenorEst->socket, carpincho3->socket);
}

void test_se_elige_por_fifo_en_caso_de_empate_2(void) {
    list_add(pcbsReady, carpincho5);
    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho4);
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho2);

    t_pcb* pcbConMenorEst = elegir_en_base_a_sjf(pcbsReady);

    CU_ASSERT_EQUAL(pcbConMenorEst->socket, carpincho5->socket);
}

void test_es_posible_actualizar_info_para_siguiente_estimacion(void) {
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho5);
    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho4);

    t_pcb* pcbConMenorEst = elegir_en_base_a_sjf(pcbsReady);
    /*
    Carpincho 1: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 2: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 3: Est = 0 + 0.5 * 3 = 1.5 <-- elige este por empate y por estar primero (fifo)
    Carpincho 4: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 5: Est = 0 + 0.5 * 3 = 1.5
    */
    CU_ASSERT_EQUAL(pcbConMenorEst->socket, carpincho3->socket);
    CU_ASSERT_EQUAL(carpincho3->sjf->estActual, 1.500000);

    // Suponiendo que llega una petición bloqueante => actualizamos la información de planificación del pcb
    pcbConMenorEst->algoritmo_update_next_est_info(pcbConMenorEst, 1000000, 0);

    pcbConMenorEst = elegir_en_base_a_sjf(pcbsReady);
    /*
    Carpincho 1: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 2: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 3: Est = 0.5 * 1.0 + 0.5 * 1.5 = 1.25 <-- elige este por menor est
    Carpincho 4: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 5: Est = 0 + 0.5 * 3 = 1.5
    */

    CU_ASSERT_EQUAL(pcbConMenorEst->socket, carpincho3->socket);
    CU_ASSERT_EQUAL(carpincho3->sjf->estActual, 1.250000);

    // Suponiendo que llega una petición bloqueante => actualizamos la información de planificación del pcb
    pcbConMenorEst->algoritmo_update_next_est_info(pcbConMenorEst, 9000000, 0);

    pcbConMenorEst = elegir_en_base_a_sjf(pcbsReady);
    /*
    Carpincho 1: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 2: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 3: Est = 0.5 * 9.0 + 0.5 * 1.25 = 5.125
    Carpincho 4: Est = 0 + 0.5 * 3 = 1.5
    Carpincho 5: Est = 0 + 0.5 * 3 = 1.5 <-- elige este por empate y por estar primero (fifo)
    */
    CU_ASSERT_EQUAL(carpincho3->sjf->estActual, 5.125000);

    CU_ASSERT_EQUAL(pcbConMenorEst->socket, carpincho5->socket);
    CU_ASSERT_EQUAL(carpincho5->sjf->estActual, 1.500000);
}
