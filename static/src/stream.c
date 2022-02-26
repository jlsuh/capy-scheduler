#include "stream.h"

void stream_send(int destSocket, void* toSend, uint32_t bufferSize) {
    uint32_t opCode = 0;
    uint32_t size = 0;
    send(destSocket, toSend, sizeof(opCode) + sizeof(size) + bufferSize, 0);
}

void buffer_send(t_buffer* buffer, uint32_t opCodeTarea, int socketConexion) {
    void* stream = get_stream_to_send(opCodeTarea, buffer);
    stream_send(socketConexion, stream, buffer->size);
    free(stream);
}

void send_empty_buffer(uint32_t opCode, int socket) {
    t_buffer* emptyBuffer = buffer_create();
    buffer_send(emptyBuffer, opCode, socket);
    buffer_destroy(emptyBuffer);
}

void recv_empty_buffer(int socket) {
    t_buffer* buffer = buffer_create();
    get_buffer(socket, buffer);
    buffer_destroy(buffer);
}

void* get_stream_to_send(uint32_t opCode, t_buffer* buffer) {
    void* toSend = malloc(sizeof(opCode) + sizeof(buffer->size) + buffer->size);
    int offset = 0;

    memcpy(toSend + offset, &opCode, sizeof(opCode));
    offset += sizeof(opCode);
    memcpy(toSend + offset, &(buffer->size), sizeof(buffer->size));
    offset += sizeof(buffer->size);
    memcpy(toSend + offset, buffer->stream, buffer->size);

    return toSend;
}

uint32_t get_op_code(int socket) {
    uint32_t opCode;
    recv(socket, &opCode, sizeof(opCode), 0);
    return opCode;
}

void get_buffer(int socket, t_buffer* destBuffer) {
    recv(socket, &(destBuffer->size), sizeof(destBuffer->size), 0);
    if (destBuffer->size > 0) {
        destBuffer->stream = malloc(destBuffer->size);
        recv(socket, destBuffer->stream, destBuffer->size, 0);
    }
}

void* stream_deserialize(int socket) {
    uint32_t opCode = get_op_code(socket);
    t_buffer* buffer = buffer_create();
    get_buffer(socket, buffer);

    void* deserialized = NULL;
    switch (opCode) {
        case STRING:
            deserialized = buffer_unpack_string(buffer);
            break;
        default:
            puts("Non valid OpCode");
            break;
    }
    buffer_destroy(buffer);
    return deserialized;  // Ante eventual falla retorna NULL
}
