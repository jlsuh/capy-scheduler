#include "tests_kernel.h"

int main(int argc, char* argv[]) {
    CU_initialize_registry();
    CU_basic_set_mode(CU_BRM_VERBOSE);

    commons_tests();
    kernel_tests();
    static_lib_tests();

    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

void commons_tests(void) {

    CU_pSuite commonsSuite = CU_add_suite_with_setup_and_teardown(
        "Commons Test Suite",
        NULL,
        NULL,
        test_commons_setup,
        test_commons_tear_down
    );
    t_test_case commonsTestCases[] = {
        TEST_FUNC(test_dictionary),
        TEST_FUNC(test_es_posible_cambiar_de_string_en_un_instante_posterior),
        TEST_FUNC(test_list_get_index),
        TEST_FUNC(test_list_remove_retorna_nulo_en_caso_de_lista_vacia),
        TEST_FUNC(test_eliminar_un_elemento_de_la_lista_de_una_queue),
    };
    ADD_TEST_CASES_TO_SUITE(commonsSuite, commonsTestCases);

}

void kernel_tests(void) {

    CU_pSuite SJFSuite = CU_add_suite_with_setup_and_teardown(
        "Algoritmo SJF Test Suite",
        NULL,
        NULL,
        test_sjf_setup,
        test_sjf_tear_down
    );
    t_test_case SJFTestCases[] = {
        TEST_FUNC(test_es_posible_actualizar_info_para_siguiente_estimacion),
        TEST_FUNC(test_la_estimacion_actual_inicial_es_la_misma_para_todos),
        TEST_FUNC(test_se_elige_por_fifo_en_caso_de_empate_1),
        TEST_FUNC(test_se_elige_por_fifo_en_caso_de_empate_2),
    };
    ADD_TEST_CASES_TO_SUITE(SJFSuite, SJFTestCases);

    CU_pSuite HRRNSuite = CU_add_suite_with_setup_and_teardown(
        "Algoritmo HRRN Test Suite",
        NULL,
        NULL,
        test_hrrn_setup,
        test_hrrn_tear_down
    );
    t_test_case HRRNTestCases[] = {
        TEST_FUNC(test_el_pcb_de_mayor_RR_es_quien_se_inicializa_primero_1),
        TEST_FUNC(test_el_pcb_de_mayor_RR_es_quien_se_inicializa_primero_2),
        TEST_FUNC(test_en_caso_de_empate_por_waiting_y_service_time_se_elige_al_primero_de_la_cola),
        TEST_FUNC(test_es_posible_elegir_el_siguiente_a_ejecutar_con_hrrn),
        TEST_FUNC(test_se_elige_al_carpincho_de_mayor_waiting_time_en_caso_de_empatar_por_service_time),
        TEST_FUNC(test_se_elige_al_carpincho_de_menor_service_time_en_caso_de_empatar_por_waiting_time),
    };
    ADD_TEST_CASES_TO_SUITE(HRRNSuite, HRRNTestCases);

    CU_pSuite deadlockSuite = CU_add_suite_with_setup_and_teardown(
        "Deadlock Test Suite",
        NULL,
        NULL,
        test_deadlock_setup,
        test_deadlock_tear_down
    );
    t_test_case deadlockTestCases[] = {
        TEST_FUNC(test_deteccion_y_recuperacion_del_deadlock_1),
        TEST_FUNC(test_deteccion_y_recuperacion_del_deadlock_2),
        TEST_FUNC(test_es_posible_eliminar_columnas_y_filas_nulas_1),
        TEST_FUNC(test_es_posible_eliminar_columnas_y_filas_nulas_2),
        TEST_FUNC(test_es_posible_eliminar_columnas_y_filas_nulas_3),
        TEST_FUNC(test_pcb_create),
        TEST_FUNC(test_sem_create),
        TEST_FUNC(test_un_proceso_al_hacer_un_post_de_un_semaforo_que_no_retenia_sigue_sin_retener_ninguna_instancia),
        TEST_FUNC(test_un_proceso_al_hacer_un_post_deja_de_tener_una_intancia_que_retenia),
        TEST_FUNC(test_un_proceso_no_retiene_una_instancia_del_semaforo_en_caso_de_bloquearse),
        TEST_FUNC(test_un_proceso_retiene_dos_instancias_de_un_semaforo),
        TEST_FUNC(test_un_proceso_retiene_una_instancia_de_un_semaforo_bloqueandose_luego_en_el_mismo),
        TEST_FUNC(test_un_proceso_retiene_una_instancia_de_un_semaforo_pero_se_bloquea_en_otro_semaforo),
        TEST_FUNC(test_un_proceso_retiene_una_instancia_del_semaforo_en_caso_de_no_bloquearse),
    };
    ADD_TEST_CASES_TO_SUITE(deadlockSuite, deadlockTestCases);

}

void static_lib_tests(void) {

    CU_pSuite bufferSuite = CU_add_suite_with_setup_and_teardown(
        "buffer.h Test Suite",
        test_suite_initialize,
        test_suite_cleanUp,
        test_buffer_setup,
        test_buffer_tear_down
    );
    t_test_case bufferTestCases[] = {
        TEST_FUNC(test_buffer_create),
        TEST_FUNC(test_es_posible_desempaquetar_cualquier_valor),
        TEST_FUNC(test_es_posible_empaquetar_cualquier_valor),
        TEST_FUNC(test_es_posible_empaquetar_y_desempaquetar_un_string),
    };
    ADD_TEST_CASES_TO_SUITE(bufferSuite, bufferTestCases);

    CU_pSuite streamConexionesSuite = CU_add_suite_with_setup_and_teardown(
        "stream.h & conexiones.h Test Suite",
        test_suite_initialize,
        test_suite_cleanUp,
        test_stream_conexiones_setup,
        test_stream_conexiones_tear_down
    );
    t_test_case streamConexionesTestCases[] = {
        TEST_FUNC(test_es_posible_establecer_una_conexion),
        TEST_FUNC(test_es_posible_serializar_un_string_enviarlo_y_deserializarlo),
    };
    ADD_TEST_CASES_TO_SUITE(streamConexionesSuite, streamConexionesTestCases);

}

// Se ejecuta cada vez que inicia 1 CU_pSuite
int test_suite_initialize(void) {
    testConfig = config_create(TESTS_CONFIG_PATH);
    return EXIT_SUCCESS;
}

// Se ejecuta cada vez que termina 1 CU_pSuite
int test_suite_cleanUp(void) {
    config_destroy(testConfig);
    return EXIT_SUCCESS;
}
