#include "stream_connections_test.h"

#include <CUnit/Basic.h>
#include <commons/config.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "buffer.h"
#include "connections.h"
#include "stream.h"

extern t_config* testConfig;  // En tests.h
static char* ip;
static char* puerto;
static int sockServ;
static int sockServDelCliente;
static int sockClienteDelServ;
static struct sockaddr sockAddr;
static socklen_t addrSize;

static char* testString;

// @Before
void test_stream_conexiones_setup(void) {
    ip = config_get_string_value(testConfig, "IP");
    puerto = config_get_string_value(testConfig, "PUERTO");
    sockServ = iniciar_servidor(ip, puerto);
    sockServDelCliente = conectar_a_servidor(ip, puerto);
    addrSize = sizeof(sockAddr);
    sockClienteDelServ = accept(sockServ, &sockAddr, &addrSize);

    testString = config_get_string_value(testConfig, "TEST_STRING");
}

// @After
void test_stream_conexiones_tear_down(void) {
    close(sockClienteDelServ);
    close(sockServDelCliente);
    close(sockServ);
}

void test_es_posible_establecer_una_conexion(void) {
    CU_ASSERT(sockClienteDelServ > 0);
}

void test_es_posible_serializar_un_string_enviarlo_y_deserializarlo(void) {
    t_buffer* senderBuffer = buffer_create();
    buffer_pack_string(senderBuffer, testString);
    stream_send_buffer(senderBuffer, 0, sockClienteDelServ);
    buffer_destroy(senderBuffer);

    t_buffer* recvdBuffer = buffer_create();
    stream_recv_op_code(sockServDelCliente);
    stream_recv_buffer(sockServDelCliente, recvdBuffer);
    char* recvdString = buffer_unpack_string(recvdBuffer);
    buffer_destroy(recvdBuffer);

    CU_ASSERT_STRING_EQUAL(recvdString, testString);

    free(recvdString);
}
