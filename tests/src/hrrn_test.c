#include "hrrn_test.h"

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
void test_hrrn_setup(void) {
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
    carpincho1 = pcb_create(ID1, "HRRN");
    carpincho2 = pcb_create(ID2, "HRRN");
    carpincho3 = pcb_create(ID3, "HRRN");
    carpincho4 = pcb_create(ID4, "HRRN");
    carpincho5 = pcb_create(ID5, "HRRN");
}

// @After
void test_hrrn_tear_down(void) {
    carpincho1->algoritmo_destroy(carpincho1);
    carpincho2->algoritmo_destroy(carpincho2);
    carpincho3->algoritmo_destroy(carpincho3);
    carpincho4->algoritmo_destroy(carpincho4);
    carpincho5->algoritmo_destroy(carpincho5);
    list_destroy(pcbsReady);
}

void test_el_pcb_de_mayor_RR_es_quien_se_inicializa_primero_1(void) {
    carpincho3->algoritmo_init(carpincho3);
    carpincho2->algoritmo_init(carpincho2);
    carpincho1->algoritmo_init(carpincho1);
    carpincho5->algoritmo_init(carpincho5);
    carpincho4->algoritmo_init(carpincho4);

    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho4);
    list_add(pcbsReady, carpincho5);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(pcbDeMayorRR->socket, carpincho3->socket);
}

void test_el_pcb_de_mayor_RR_es_quien_se_inicializa_primero_2(void) {
    carpincho5->algoritmo_init(carpincho5);
    carpincho2->algoritmo_init(carpincho2);
    carpincho4->algoritmo_init(carpincho4);
    carpincho3->algoritmo_init(carpincho3);
    carpincho1->algoritmo_init(carpincho1);

    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho4);
    list_add(pcbsReady, carpincho5);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(pcbDeMayorRR->socket, carpincho5->socket);
}

void test_es_posible_elegir_el_siguiente_a_ejecutar_con_hrrn(void) {
    carpincho5->algoritmo_init(carpincho5);
    carpincho4->algoritmo_init(carpincho4);
    carpincho1->algoritmo_init(carpincho1);
    carpincho3->algoritmo_init(carpincho3);
    carpincho2->algoritmo_init(carpincho2);

    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho4);
    list_add(pcbsReady, carpincho5);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(pcbDeMayorRR->socket, carpincho5->socket);
    CU_ASSERT_EQUAL(pcbDeMayorRR->hrrn->s, /*s = 0.5 * 0 + 0.5 * 3 =*/1.50000 /*= 1.5UT*/);

    // Suponiendo que ejecuta 2UT
    carpincho5->algoritmo_update_next_est_info(carpincho5, /*Real anterior =*/2000000 /*= 2UT*/, 0);
    CU_ASSERT_EQUAL(carpincho5->hrrn->s, /* s = 0.5 * 2 + 0.5 * 1.5 =*/1.75000);

    pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(pcbDeMayorRR->socket, carpincho4->socket);
    CU_ASSERT_EQUAL(carpincho4->hrrn->s, 1.50000);
}

void test_se_elige_al_carpincho_de_menor_service_time_en_caso_de_empatar_por_waiting_time(void) {
    /*  Si w1 == w2:
        RR1 > RR2 <=> w1 / s1 + 1 > w2 / s2 + 1 <=> s1 < s2
    */
    clock_t now = clock();

    carpincho1->algoritmo_init(carpincho1);
    carpincho2->algoritmo_init(carpincho2);
    carpincho3->algoritmo_init(carpincho3);
    carpincho4->algoritmo_init(carpincho4);
    carpincho5->algoritmo_init(carpincho5);

    carpincho1->hrrn->w = now;
    carpincho2->hrrn->w = now;
    carpincho3->hrrn->w = now;
    carpincho4->hrrn->w = now;
    carpincho5->hrrn->w = now;

    double sameServiceTime = 1.000001;

    carpincho1->hrrn->s = sameServiceTime;
    carpincho2->hrrn->s = sameServiceTime;
    carpincho3->hrrn->s = sameServiceTime;
    carpincho4->hrrn->s = 1.000000;
    carpincho5->hrrn->s = sameServiceTime;

    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho5);
    list_add(pcbsReady, carpincho4);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(pcbDeMayorRR->socket, carpincho4->socket);
}

void test_se_elige_al_carpincho_de_mayor_waiting_time_en_caso_de_empatar_por_service_time(void) {
    /*  Si s1 == s2:
        RR1 > RR2 <=> w1 / s1 + 1 > w2 / s2 + 1 <=> w1 > w2
    */
    clock_t now = 1000000;

    carpincho1->algoritmo_init(carpincho1);
    carpincho2->algoritmo_init(carpincho2);
    carpincho3->algoritmo_init(carpincho3);
    carpincho4->algoritmo_init(carpincho4);
    carpincho5->algoritmo_init(carpincho5);

    double sameServiceTime = 1.000000;

    carpincho1->hrrn->s = sameServiceTime;
    carpincho2->hrrn->s = sameServiceTime;
    carpincho3->hrrn->s = sameServiceTime;
    carpincho4->hrrn->s = sameServiceTime;
    carpincho5->hrrn->s = sameServiceTime;

    carpincho1->hrrn->w = now;
    carpincho2->hrrn->w = 999999;
    carpincho3->hrrn->w = now;
    carpincho4->hrrn->w = now;
    carpincho5->hrrn->w = now;

    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho5);
    list_add(pcbsReady, carpincho4);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(pcbDeMayorRR->socket, carpincho2->socket);
}

void test_en_caso_de_empate_por_waiting_y_service_time_se_elige_al_primero_de_la_cola(void) {
    clock_t now = 1000000;

    carpincho1->algoritmo_init(carpincho1);
    carpincho2->algoritmo_init(carpincho2);
    carpincho3->algoritmo_init(carpincho3);
    carpincho4->algoritmo_init(carpincho4);
    carpincho5->algoritmo_init(carpincho5);

    double sameServiceTime = 1.000000;

    carpincho1->hrrn->s = sameServiceTime;
    carpincho2->hrrn->s = sameServiceTime;
    carpincho3->hrrn->s = sameServiceTime;
    carpincho4->hrrn->s = sameServiceTime;
    carpincho5->hrrn->s = sameServiceTime;

    carpincho1->hrrn->w = now;
    carpincho2->hrrn->w = now;
    carpincho3->hrrn->w = now;
    carpincho4->hrrn->w = now;
    carpincho5->hrrn->w = now;

    list_add(pcbsReady, carpincho2);
    list_add(pcbsReady, carpincho5);
    list_add(pcbsReady, carpincho3);
    list_add(pcbsReady, carpincho1);
    list_add(pcbsReady, carpincho4);

    t_pcb* pcbDeMayorRR = elegir_en_base_a_hrrn(pcbsReady);

    CU_ASSERT_EQUAL(pcbDeMayorRR->socket, carpincho2->socket);
}
