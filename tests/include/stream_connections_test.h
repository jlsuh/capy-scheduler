#ifndef STREAM_CONNECTIONS_TEST_H_INCLUDED
#define STREAM_CONNECTIONS_TEST_H_INCLUDED

#include <CUnit/Basic.h>
#include <commons/config.h>
#include <sys/types.h>

#include "connections.h"
#include "stream.h"

void test_stream_conexiones_setup(void);
void test_stream_conexiones_tear_down(void);

void test_es_posible_establecer_una_conexion(void);
void test_es_posible_serializar_un_string_enviarlo_y_deserializarlo(void);

#endif
