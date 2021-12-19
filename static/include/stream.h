#ifndef STREAM_H_INCLUDED
#define STREAM_H_INCLUDED

#include <netdb.h>
#include <stdio.h>

#include "buffer.h"

typedef enum {
    STRING,
} op_code;

uint32_t get_op_code(int socket);
void *get_stream_to_send(uint32_t opCode, t_buffer *buffer);
void *stream_deserialize(int socket);
void buffer_send(t_buffer *buffer, uint32_t opCodeTarea, int socketConexion);
void get_buffer(int socket, t_buffer *destBuffer);
void recv_empty_buffer(int socket);
void send_empty_buffer(uint32_t opCode, int socket);
void stream_send(int destSocket, void *toSend, uint32_t bufferSize);

#endif