#include "hrrn_test.h"

#include <CUnit/Basic.h>
#include <commons/collections/list.h>
#include <stdint.h>
#include <stdlib.h>

#include "domain/cola_planificacion.h"
#include "domain/pcb.h"
#include "scheduler.h"

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
static char* ALGORITHM = "HRRN";

// @Before
void test_hrrn_setup(void) {
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
}

// @After
void test_hrrn_tear_down(void) {
    pcb_algoritmo_destroy(carpincho1);
    pcb_algoritmo_destroy(carpincho2);
    pcb_algoritmo_destroy(carpincho3);
    pcb_algoritmo_destroy(carpincho4);
    pcb_algoritmo_destroy(carpincho5);
    cola_planificacion_destroy(pcbsReady);
}

void test_se_elige_al_carpincho_de_menor_service_time_en_caso_de_empatar_por_waiting_time(void) {
    /*  Si w1 == w2:
        RR1 > RR2 <=> w1 / s1 + 1 > w2 / s2 + 1 <=> s1 < s2
    */
    clock_t now = clock();

    pcb_algoritmo_init(carpincho1);
    pcb_algoritmo_init(carpincho2);
    pcb_algoritmo_init(carpincho3);
    pcb_algoritmo_init(carpincho4);
    pcb_algoritmo_init(carpincho5);

    pcb_set_waiting_time(carpincho1, now);
    pcb_set_waiting_time(carpincho2, now);
    pcb_set_waiting_time(carpincho3, now);
    pcb_set_waiting_time(carpincho4, now);
    pcb_set_waiting_time(carpincho5, now);

    double sameServiceTime = 1.000001;

    pcb_set_service_time(carpincho1, sameServiceTime);
    pcb_set_service_time(carpincho2, sameServiceTime);
    pcb_set_service_time(carpincho3, sameServiceTime);
    pcb_set_service_time(carpincho4, 1.000000);
    pcb_set_service_time(carpincho5, sameServiceTime);

    t_list* readyList = cola_planificacion_get_list(pcbsReady);
    list_add(readyList, carpincho3);
    list_add(readyList, carpincho2);
    list_add(readyList, carpincho1);
    list_add(readyList, carpincho5);
    list_add(readyList, carpincho4);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(*pcb_get_socket(pcbDeMayorRR), *pcb_get_socket(carpincho4));
}

void test_se_elige_al_carpincho_de_mayor_waiting_time_en_caso_de_empatar_por_service_time(void) {
    /*  Si s1 == s2:
        RR1 > RR2 <=> w1 / s1 + 1 > w2 / s2 + 1 <=> w1 > w2
    */
    clock_t now = 1000000;

    pcb_algoritmo_init(carpincho1);
    pcb_algoritmo_init(carpincho2);
    pcb_algoritmo_init(carpincho3);
    pcb_algoritmo_init(carpincho4);
    pcb_algoritmo_init(carpincho5);

    double sameServiceTime = 1.000000;

    pcb_set_service_time(carpincho1, sameServiceTime);
    pcb_set_service_time(carpincho2, sameServiceTime);
    pcb_set_service_time(carpincho3, sameServiceTime);
    pcb_set_service_time(carpincho4, sameServiceTime);
    pcb_set_service_time(carpincho5, sameServiceTime);

    pcb_set_waiting_time(carpincho1, now);
    pcb_set_waiting_time(carpincho2, 999999);
    pcb_set_waiting_time(carpincho3, now);
    pcb_set_waiting_time(carpincho4, now);
    pcb_set_waiting_time(carpincho5, now);

    t_list* readyList = cola_planificacion_get_list(pcbsReady);
    list_add(readyList, carpincho3);
    list_add(readyList, carpincho2);
    list_add(readyList, carpincho1);
    list_add(readyList, carpincho5);
    list_add(readyList, carpincho4);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(*pcb_get_socket(pcbDeMayorRR), *pcb_get_socket(carpincho2));
}

void test_en_caso_de_empate_por_waiting_y_service_time_se_elige_al_primero_de_la_cola(void) {
    clock_t now = 1000000;

    pcb_algoritmo_init(carpincho1);
    pcb_algoritmo_init(carpincho2);
    pcb_algoritmo_init(carpincho3);
    pcb_algoritmo_init(carpincho4);
    pcb_algoritmo_init(carpincho5);

    double sameServiceTime = 1.000000;

    pcb_set_service_time(carpincho1, sameServiceTime);
    pcb_set_service_time(carpincho2, sameServiceTime);
    pcb_set_service_time(carpincho3, sameServiceTime);
    pcb_set_service_time(carpincho4, sameServiceTime);
    pcb_set_service_time(carpincho5, sameServiceTime);

    pcb_set_waiting_time(carpincho1, now);
    pcb_set_waiting_time(carpincho2, now);
    pcb_set_waiting_time(carpincho3, now);
    pcb_set_waiting_time(carpincho4, now);
    pcb_set_waiting_time(carpincho5, now);

    t_list* readyList = cola_planificacion_get_list(pcbsReady);
    list_add(readyList, carpincho2);
    list_add(readyList, carpincho5);
    list_add(readyList, carpincho3);
    list_add(readyList, carpincho1);
    list_add(readyList, carpincho4);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(*pcb_get_socket(pcbDeMayorRR), *pcb_get_socket(carpincho2));
}
