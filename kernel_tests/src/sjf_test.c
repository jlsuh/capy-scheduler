#include "sjf_test.h"

#include <CUnit/Basic.h>
#include <commons/collections/list.h>
#include <stdint.h>
#include <stdlib.h>

#include "domain/cola_planificacion.h"
#include "domain/pcb.h"
#include "scheduler.h"

static double ESTIMACION_INICIAL;
static double ALFA;
static t_cola_planificacion* pcbsReady;
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
static char* ALGORITHM = "SJF";

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

    pcbsReady = cola_planificacion_create(0);
    carpincho1 = pcb_create(ID1, *ID1, ALGORITHM);
    carpincho2 = pcb_create(ID2, *ID2, ALGORITHM);
    carpincho3 = pcb_create(ID3, *ID3, ALGORITHM);
    carpincho4 = pcb_create(ID4, *ID4, ALGORITHM);
    carpincho5 = pcb_create(ID5, *ID5, ALGORITHM);

    pcb_algoritmo_init(carpincho1);
    pcb_algoritmo_init(carpincho2);
    pcb_algoritmo_init(carpincho3);
    pcb_algoritmo_init(carpincho4);
    pcb_algoritmo_init(carpincho5);
}

// @After
void test_sjf_tear_down(void) {
    pcb_algoritmo_destroy(carpincho1);
    pcb_algoritmo_destroy(carpincho2);
    pcb_algoritmo_destroy(carpincho3);
    pcb_algoritmo_destroy(carpincho4);
    pcb_algoritmo_destroy(carpincho5);
    cola_planificacion_destroy(pcbsReady);
}

void test_la_estimacion_actual_inicial_es_la_misma_para_todos(void) {
    t_list* readyList = cola_planificacion_get_list(pcbsReady);
    list_add(readyList, carpincho3);
    list_add(readyList, carpincho2);
    list_add(readyList, carpincho1);
    list_add(readyList, carpincho5);
    list_add(readyList, carpincho4);

    double est1 = pcb_get_est_actual(carpincho1);
    double est2 = pcb_get_est_actual(carpincho2);
    double est3 = pcb_get_est_actual(carpincho3);
    double est4 = pcb_get_est_actual(carpincho4);
    double est5 = pcb_get_est_actual(carpincho5);

    CU_ASSERT_EQUAL(est1, est2);
    CU_ASSERT_EQUAL(est2, est3);
    CU_ASSERT_EQUAL(est3, est4);
    CU_ASSERT_EQUAL(est4, est5);
}

void test_se_elige_por_fifo_en_caso_de_empate_1(void) {
    t_list* readyList = cola_planificacion_get_list(pcbsReady);
    list_add(readyList, carpincho3);
    list_add(readyList, carpincho2);
    list_add(readyList, carpincho1);

    t_pcb* pcbConMenorEst = elegir_en_base_a_sjf(pcbsReady);

    CU_ASSERT_EQUAL(*pcb_get_socket(pcbConMenorEst), *pcb_get_socket(carpincho3));
}

void test_se_elige_por_fifo_en_caso_de_empate_2(void) {
    t_list* readyList = cola_planificacion_get_list(pcbsReady);
    list_add(readyList, carpincho5);
    list_add(readyList, carpincho1);
    list_add(readyList, carpincho4);
    list_add(readyList, carpincho3);
    list_add(readyList, carpincho2);

    t_pcb* pcbConMenorEst = elegir_en_base_a_sjf(pcbsReady);

    CU_ASSERT_EQUAL(*pcb_get_socket(pcbConMenorEst), *pcb_get_socket(carpincho5));
}
