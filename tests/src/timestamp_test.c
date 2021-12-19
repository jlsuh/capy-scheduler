#include "timestamp_test.h"

// @Before
void test_timeStamp_setup(void) {

}

// @After
void test_timeStamp_tear_down(void) {

}

void test_timeStamp_usage(void) {
    const int tamanio = 10;
    double muestrasRn[tamanio];

    for(int i = 0; i < tamanio; i++) {    
        clock_t ti = clock();
        sleep(1);
        sleep(1);
        sleep(1);
        clock_t tf = clock();
        muestrasRn[i] = (double)(tf - ti) / CLOCKS_PER_SEC;
    }

    // CU_ASSERT_DOUBLE_EQUAL(realEjecutado, 0.003, 3);
    printf("\n");
    for(int i = 0; i < tamanio; i++) {
        printf("Interval: %.10f\n", muestrasRn[i]);
    }
}
