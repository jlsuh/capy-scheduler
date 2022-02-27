#ifndef SJF_TEST_H_INCLUDED
#define SJF_TEST_H_INCLUDED

void test_sjf_setup(void);
void test_sjf_tear_down(void);

void test_es_posible_actualizar_info_para_siguiente_estimacion(void);
void test_la_estimacion_actual_inicial_es_la_misma_para_todos(void);
void test_se_elige_por_fifo_en_caso_de_empate_1(void);
void test_se_elige_por_fifo_en_caso_de_empate_2(void);

#endif
