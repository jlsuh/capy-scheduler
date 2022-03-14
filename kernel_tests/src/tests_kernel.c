#include <CUnit/Basic.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdlib.h>

#include "buffer_test.h"
#include "commons_test.h"
#include "hrrn_test.h"
#include "kernel_config.h"
#include "sjf_test.h"
#include "stream_connections_test.h"

#define TESTS_CONFIG_PATH "cfg/test_config.cfg"
#define KERNEL_LOG_DEST "bin/kernel_tests.log"
#define KERNEL_MODULE_NAME "Kernel Test"

#define TEST_FUNC(func) \
    { "\e[1;92m" #func "\e[0m", func }
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof(*(array)))
#define ADD_TEST_CASES_TO_SUITE(suite, tests)               \
    for (unsigned long i = 0; i < ARRAY_LENGTH(tests); i++) \
        CU_add_test(suite, tests[i].name, tests[i].func);

typedef struct {
    const char* name;
    void (*func)(void);
} t_test_case;

t_config* testConfig;
t_kernel_config* kernelCfg;
t_log* kernelLogger;

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

void commons_tests(void) {
    CU_pSuite commonsSuite = CU_add_suite_with_setup_and_teardown(
        "Commons Test Suite",
        NULL,
        NULL,
        test_commons_setup,
        test_commons_tear_down);
    t_test_case commonsTestCases[] = {
        TEST_FUNC(test_dictionary),
        TEST_FUNC(test_es_posible_cambiar_de_string_en_un_instante_posterior),
        TEST_FUNC(test_list_get_index),
        TEST_FUNC(test_list_remove_retorna_nulo_en_caso_de_lista_vacia),
    };
    ADD_TEST_CASES_TO_SUITE(commonsSuite, commonsTestCases);
}

void kernel_tests(void) {
    CU_pSuite SJFSuite = CU_add_suite_with_setup_and_teardown(
        "Algoritmo SJF Test Suite",
        NULL,
        NULL,
        test_sjf_setup,
        test_sjf_tear_down);
    t_test_case SJFTestCases[] = {
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
        test_hrrn_tear_down);
    t_test_case HRRNTestCases[] = {
        TEST_FUNC(test_en_caso_de_empate_por_waiting_y_service_time_se_elige_al_primero_de_la_cola),
        TEST_FUNC(test_se_elige_al_carpincho_de_mayor_waiting_time_en_caso_de_empatar_por_service_time),
        TEST_FUNC(test_se_elige_al_carpincho_de_menor_service_time_en_caso_de_empatar_por_waiting_time),
    };
    ADD_TEST_CASES_TO_SUITE(HRRNSuite, HRRNTestCases);
}

void static_lib_tests(void) {
    CU_pSuite bufferSuite = CU_add_suite_with_setup_and_teardown(
        "buffer.h Test Suite",
        test_suite_initialize,
        test_suite_cleanUp,
        test_buffer_setup,
        test_buffer_tear_down);
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
        test_stream_conexiones_tear_down);
    t_test_case streamConexionesTestCases[] = {
        TEST_FUNC(test_es_posible_establecer_una_conexion),
        TEST_FUNC(test_es_posible_serializar_un_string_enviarlo_y_deserializarlo),
    };
    ADD_TEST_CASES_TO_SUITE(streamConexionesSuite, streamConexionesTestCases);
}

int main(int argc, char* argv[]) {
    CU_initialize_registry();
    CU_basic_set_mode(CU_BRM_VERBOSE);

    bool activeConsole = true;
    kernelLogger = log_create(KERNEL_LOG_DEST, KERNEL_MODULE_NAME, activeConsole, LOG_LEVEL_INFO);
    kernelCfg = kernel_config_create("../kernel/cfg/kernel_config.cfg");

    commons_tests();
    kernel_tests();
    static_lib_tests();

    CU_basic_run_tests();
    CU_cleanup_registry();

    liberar_modulo_kernel(kernelLogger, kernelCfg);

    return CU_get_error();
}
