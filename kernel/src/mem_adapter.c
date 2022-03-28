#include "mem_adapter.h"

#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

#include "common_flags.h"
#include "kernel.h"
#include "kernel_config.h"
#include "stream.h"

extern t_log* kernelLogger;
extern t_kernel_config* kernelCfg;
extern pthread_mutex_t mutexMemSocket;

static bool __es_deploy_mode(void) {
    return strcmp(EXEC_MODE, "DEPLOY_MODE") == 0;
}

void enviar_memalloc_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    int32_t const memSocket = kernel_config_get_mem_socket(kernelCfg);
    pthread_mutex_lock(&mutexMemSocket);
    if (__es_deploy_mode()) {
        stream_send_buffer(memSocket, MEM_ALLOC, buffer);
        log_info(kernelLogger, "MEM_ALLOC <Carpincho %d>: Carpincho->Memoria exitosa", pcb_get_pid(pcb));

        uint32_t response = stream_recv_op_code(memSocket);

        if (OK_CONTINUE == response) {
            t_buffer* responseBuffer = buffer_create();
            stream_recv_buffer(memSocket, responseBuffer);
            stream_send_buffer(*pcb_get_socket(pcb), OK_CONTINUE, responseBuffer);
            buffer_destroy(responseBuffer);
            log_info(kernelLogger, "MEM_ALLOC <Carpincho %d>: Memoria->Carpincho exitosa", pcb_get_pid(pcb));
        } else {
            stream_recv_empty_buffer(memSocket);
            stream_send_empty_buffer(*pcb_get_socket(pcb), FAIL);
            log_error(kernelLogger, "MEM_ALLOC <Carpincho %d>: Memoria->Carpincho fallida", pcb_get_pid(pcb));
        }
    } else {
        uint32_t bytesAAllocar_Stub;
        uint32_t pid;
        buffer_unpack(buffer, &pid, sizeof(pid));
        buffer_unpack(buffer, &bytesAAllocar_Stub, sizeof(bytesAAllocar_Stub));
        buffer_pack(buffer, &bytesAAllocar_Stub, sizeof(bytesAAllocar_Stub));
        stream_send_buffer(*pcb_get_socket(pcb), OK_CONTINUE, buffer);
        log_info(kernelLogger, "MEM_ALLOC <Carpincho %d>: Se allocan %d Bytes", pcb_get_pid(pcb), bytesAAllocar_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_memwrite_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    int32_t const memSocket = kernel_config_get_mem_socket(kernelCfg);
    pthread_mutex_lock(&mutexMemSocket);
    if (__es_deploy_mode()) {
        stream_send_buffer(memSocket, MEM_WRITE, buffer);
        log_info(kernelLogger, "MEM_WRITE <Carpincho %d>: Carpincho->Memoria exitosa", pcb_get_pid(pcb));

        uint32_t response = stream_recv_op_code(memSocket);

        if (OK_CONTINUE == response) {
            t_buffer* responseBuffer = buffer_create();
            stream_recv_buffer(memSocket, responseBuffer);
            stream_send_buffer(*pcb_get_socket(pcb), OK_CONTINUE, responseBuffer);
            buffer_destroy(responseBuffer);
            log_info(kernelLogger, "MEM_WRITE <Carpincho %d>: Memoria->Carpincho exitosa", pcb_get_pid(pcb));
        } else {
            stream_recv_empty_buffer(memSocket);
            stream_send_empty_buffer(*pcb_get_socket(pcb), FAIL);
            log_error(kernelLogger, "MEM_WRITE <Carpincho %d>: Memoria->Carpincho fallida", pcb_get_pid(pcb));
        }
    } else {
        int32_t matePointerAEscribir_Stub;
        uint32_t cantidadBytesAEscribir_Stub;
        uint32_t pid;
        buffer_unpack(buffer, &pid, sizeof(pid));
        buffer_unpack(buffer, &matePointerAEscribir_Stub, sizeof(matePointerAEscribir_Stub));
        buffer_unpack(buffer, &cantidadBytesAEscribir_Stub, sizeof(cantidadBytesAEscribir_Stub));
        void* contenidoAEscribir_Stub = malloc(cantidadBytesAEscribir_Stub);
        buffer_unpack(buffer, contenidoAEscribir_Stub, cantidadBytesAEscribir_Stub);
        buffer_pack(buffer, &matePointerAEscribir_Stub, sizeof(matePointerAEscribir_Stub));
        buffer_pack(buffer, &cantidadBytesAEscribir_Stub, sizeof(cantidadBytesAEscribir_Stub));
        buffer_pack(buffer, contenidoAEscribir_Stub, cantidadBytesAEscribir_Stub);
        stream_send_buffer(*pcb_get_socket(pcb), OK_CONTINUE, buffer);
        free(contenidoAEscribir_Stub);
        log_info(kernelLogger, "MEM_WRITE <Carpincho %d>: Se escriben %d Bytes de dirección lógica %d", pcb_get_pid(pcb), cantidadBytesAEscribir_Stub, matePointerAEscribir_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_memread_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    int32_t const memSocket = kernel_config_get_mem_socket(kernelCfg);
    pthread_mutex_lock(&mutexMemSocket);
    if (__es_deploy_mode()) {
        stream_send_buffer(memSocket, MEM_READ, buffer);
        log_info(kernelLogger, "MEM_READ <Carpincho %d>: Carpincho->Memoria exitosa", pcb_get_pid(pcb));

        uint32_t response = stream_recv_op_code(memSocket);

        if (OK_CONTINUE == response) {
            t_buffer* responseBuffer = buffer_create();
            stream_recv_buffer(memSocket, responseBuffer);
            stream_send_buffer(*pcb_get_socket(pcb), OK_CONTINUE, responseBuffer);
            buffer_destroy(responseBuffer);
            log_info(kernelLogger, "MEM_READ <Carpincho %d>: Memoria->Carpincho exitosa", pcb_get_pid(pcb));
        } else {
            stream_recv_empty_buffer(memSocket);
            stream_send_empty_buffer(*pcb_get_socket(pcb), FAIL);
            log_error(kernelLogger, "MEM_READ <Carpincho %d>: Memoria->Carpincho fallida", pcb_get_pid(pcb));
        }
    } else {
        int32_t matePointerALeer_Stub;
        uint32_t cantidadBytesALeer_Stub;
        uint32_t pid;
        buffer_unpack(buffer, &pid, sizeof(pid));
        buffer_unpack(buffer, &matePointerALeer_Stub, sizeof(matePointerALeer_Stub));
        buffer_unpack(buffer, &cantidadBytesALeer_Stub, sizeof(cantidadBytesALeer_Stub));
        buffer_pack(buffer, &matePointerALeer_Stub, sizeof(matePointerALeer_Stub));
        buffer_pack(buffer, &cantidadBytesALeer_Stub, sizeof(cantidadBytesALeer_Stub));
        stream_send_buffer(*pcb_get_socket(pcb), OK_CONTINUE, buffer);
        log_info(kernelLogger, "MEM_READ <Carpincho %d>: Se leen %d Bytes de dirección lógica %d", pcb_get_pid(pcb), cantidadBytesALeer_Stub, matePointerALeer_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_memfree_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    int32_t const memSocket = kernel_config_get_mem_socket(kernelCfg);
    pthread_mutex_lock(&mutexMemSocket);
    if (__es_deploy_mode()) {
        stream_send_buffer(memSocket, MEM_FREE, buffer);
        log_info(kernelLogger, "MEM_FREE <Carpincho %d>: Carpincho->Memoria exitosa", pcb_get_pid(pcb));

        uint32_t response = stream_recv_op_code(memSocket);
        stream_recv_empty_buffer(memSocket);

        if (OK_CONTINUE == response) {
            stream_send_empty_buffer(*pcb_get_socket(pcb), OK_CONTINUE);
            log_info(kernelLogger, "MEM_FREE <Carpincho %d>: Memoria->Carpincho exitosa", pcb_get_pid(pcb));
        } else {
            stream_send_empty_buffer(*pcb_get_socket(pcb), FAIL);
            log_error(kernelLogger, "MEM_FREE <Carpincho %d>: Memoria->Carpincho fallida", pcb_get_pid(pcb));
        }
    } else {
        int32_t matePointerALiberar_Stub;
        uint32_t pid;
        buffer_unpack(buffer, &pid, sizeof(pid));
        buffer_unpack(buffer, &matePointerALiberar_Stub, sizeof(matePointerALiberar_Stub));
        stream_send_empty_buffer(*pcb_get_socket(pcb), OK_CONTINUE);
        log_info(kernelLogger, "MEM_FREE <Carpincho %d>: Se libera espacio de memoria de dirección lógica %d", pcb_get_pid(pcb), matePointerALiberar_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_mate_close_a_memoria(t_pcb* pcb) {
    int32_t const memSocket = kernel_config_get_mem_socket(kernelCfg);
    pthread_mutex_lock(&mutexMemSocket);
    if (__es_deploy_mode()) {
        t_buffer* closeBuffer = buffer_create();
        uint32_t pid = pcb_get_pid(pcb);
        buffer_pack(closeBuffer, &pid, sizeof(pid));
        stream_send_buffer(memSocket, MATE_CLOSE, closeBuffer);

        uint32_t response = stream_recv_op_code(memSocket);

        if (OK_CONTINUE == response) {
            stream_recv_empty_buffer(memSocket);
        } else {
            log_error(kernelLogger, "EXIT: Error al intentar finalizar Carpincho ID %d de Memoria", pcb_get_pid(pcb));
            exit(0);
        }
    }
    log_info(kernelLogger, "EXIT: Se finaliza Carpincho ID %d", pcb_get_pid(pcb));
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_suspension_de_carpincho_a_memoria(t_pcb* pcb) {
    int32_t const memSocket = kernel_config_get_mem_socket(kernelCfg);
    if (__es_deploy_mode()) {
        pthread_mutex_lock(&mutexMemSocket);
        t_buffer* closeBuffer = buffer_create();
        uint32_t pid = pcb_get_pid(pcb);
        buffer_pack(closeBuffer, &pid, sizeof(pid));
        stream_send_buffer(memSocket, SUSPEND_CARPINCHO, closeBuffer);

        uint32_t response = stream_recv_op_code(memSocket);

        if (OK_CONTINUE == response) {
            stream_recv_empty_buffer(memSocket);
        } else {
            log_error(kernelLogger, "Suspensión: Error al intentar suspender un Carpincho ID %d de Memoria", pcb_get_pid(pcb));
            exit(0);
        }
    }
    log_info(kernelLogger, "Suspensión: Se suspende Carpincho ID %d", pcb_get_pid(pcb));
    pthread_mutex_unlock(&mutexMemSocket);
}
