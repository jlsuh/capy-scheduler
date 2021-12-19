#ifndef HRRN_TEST_H_INCLUDED
#define HRRN_TEST_H_INCLUDED

#include <CUnit/Basic.h>
#include <stdlib.h>

#include "duplicated_logic_allocator.h"
#include "kernel_structs.h"

void test_hrrn_setup(void);
void test_hrrn_tear_down(void);

void test_el_pcb_de_mayor_RR_es_quien_se_inicializa_primero_1(void);
void test_el_pcb_de_mayor_RR_es_quien_se_inicializa_primero_2(void);
void test_en_caso_de_empate_por_waiting_y_service_time_se_elige_al_primero_de_la_cola(void);
void test_es_posible_elegir_el_siguiente_a_ejecutar_con_hrrn(void);
void test_se_elige_al_carpincho_de_mayor_waiting_time_en_caso_de_empatar_por_service_time(void);
void test_se_elige_al_carpincho_de_menor_service_time_en_caso_de_empatar_por_waiting_time(void);

#endif
