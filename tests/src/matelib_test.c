#include "matelib_test.h"

static mate_instance mateRef;

static t_config* testConfig;
static char* ip;
static char* puerto;
static int sockServ;
static int sockServDelCliente;
static int sockClienteDelServ;
static struct sockaddr sockAddr;
static socklen_t addrSize;

static int mateInitVal;

// @Before
void test_matelib_setup(void) {
    testConfig = config_create(MATELIB_TEST_CFG_PATH);
    ip = config_get_string_value(testConfig, "IP");
    puerto = config_get_string_value(testConfig, "PUERTO");

    sockServ = iniciar_servidor(ip, puerto);
    sockServDelCliente = conectar_a_servidor(ip, puerto);
    addrSize = sizeof(sockAddr);
    sockClienteDelServ = accept(sockServ, &sockAddr, &addrSize);
    config_destroy(testConfig);

    mateInitVal = mate_init(&mateRef, MATELIB_TEST_CFG_PATH);
}

// @After
void test_matelib_tear_down(void) {
    mate_close(&mateRef);
    close(sockClienteDelServ);
    close(sockServDelCliente);
    close(sockServ);
}

void test_mate_init(void) {
    CU_ASSERT_EQUAL(mateInitVal, 1);
}
