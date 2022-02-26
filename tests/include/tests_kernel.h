#ifndef TESTS_KERNEL_H_INCLUDED
#define TESTS_KERNEL_H_INCLUDED

#include "buffer_test.h"
#include "commons_test.h"
#include "deadlock_test.h"
#include "hrrn_test.h"
#include "sjf_test.h"
#include "stream_connections_test.h"

#define TESTS_CONFIG_PATH "cfg/test_config.cfg"

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

int test_suite_cleanUp(void);
int test_suite_initialize(void);

void commons_tests(void);
void kernel_tests(void);
void static_lib_tests(void);

t_config* testConfig;

#endif
