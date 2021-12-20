#include "buffer.h"

t_buffer* buffer_create() {
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = 0;
    buffer->stream = NULL;
    return buffer;
}

void buffer_destroy(t_buffer* buffer) {
    free(buffer->stream);
    free(buffer);
}

void buffer_pack(t_buffer* buffer, void* streamToAdd, int size) {
    buffer->stream = realloc(buffer->stream, buffer->size + size);
    memcpy(buffer->stream + buffer->size, streamToAdd, size);
    buffer->size += size;
}

void buffer_unpack(t_buffer* buffer, void* dest, int size) {
    memcpy(dest, buffer->stream, size);
    buffer->size -= size;
    memmove(buffer->stream, buffer->stream + size, buffer->size);
    buffer->stream = realloc(buffer->stream, buffer->size);
}

void buffer_pack_string(t_buffer* buffer, char* stringToAdd) {
    uint32_t length = strlen(stringToAdd) + 1;
    buffer_pack(buffer, &length, sizeof(length));
    buffer->stream = realloc(buffer->stream, buffer->size + length);
    memcpy(buffer->stream + buffer->size, stringToAdd, length);
    buffer->size += length;
}

char* buffer_unpack_string(t_buffer* buffer) {
    char* str;
    uint32_t length;
    buffer_unpack(buffer, &length, sizeof(length));
    str = malloc(length);
    buffer_unpack(buffer, str, length);
    return str;
}
