#include "mem_adapter.h"

bool es_deploy_mode(void) {
    return strcmp(EXEC_MODE, "DEPLOY_MODE") == 0;
}

void enviar_memalloc_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    pthread_mutex_lock(&mutexMemSocket);
    if(es_deploy_mode()) {
        buffer_send(buffer, MEM_ALLOC, kernelCfg->MEMORIA_SOCKET);
        log_info(kernelLogger, "MEM_ALLOC <Carpincho %d>: Carpincho->Memoria exitosa", pcb->pid);

        uint32_t response = get_op_code(kernelCfg->MEMORIA_SOCKET);

        if(response == OK_CONTINUE) {
            t_buffer* responseBuffer = buffer_create();
            get_buffer(kernelCfg->MEMORIA_SOCKET, responseBuffer);
            buffer_send(responseBuffer, OK_CONTINUE, *(pcb->socket));
            buffer_destroy(responseBuffer);
            log_info(kernelLogger, "MEM_ALLOC <Carpincho %d>: Memoria->Carpincho exitosa", pcb->pid);
        } else {
            recv_empty_buffer(kernelCfg->MEMORIA_SOCKET);
            send_empty_buffer(FAIL, *(pcb->socket));
            log_error(kernelLogger, "MEM_ALLOC <Carpincho %d>: Memoria->Carpincho fallida", pcb->pid);
        }
    } else {
        uint32_t bytesAAllocar_Stub;
        uint32_t pid;
        buffer_unpack(buffer, &pid, sizeof(pid));
        buffer_unpack(buffer, &bytesAAllocar_Stub, sizeof(bytesAAllocar_Stub));
        buffer_pack(buffer, &bytesAAllocar_Stub, sizeof(bytesAAllocar_Stub));
        buffer_send(buffer, OK_CONTINUE, *(pcb->socket));
        log_info(kernelLogger, "MEM_ALLOC <Carpincho %d>: Se allocan %d Bytes", pcb->pid, bytesAAllocar_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_memwrite_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    pthread_mutex_lock(&mutexMemSocket);
    if(es_deploy_mode()) {
        buffer_send(buffer, MEM_WRITE, kernelCfg->MEMORIA_SOCKET);
        log_info(kernelLogger, "MEM_WRITE <Carpincho %d>: Carpincho->Memoria exitosa", pcb->pid);

        uint32_t response = get_op_code(kernelCfg->MEMORIA_SOCKET);

        if(response == OK_CONTINUE) {
            t_buffer* responseBuffer = buffer_create();
            get_buffer(kernelCfg->MEMORIA_SOCKET, responseBuffer);
            buffer_send(responseBuffer, OK_CONTINUE, *(pcb->socket));
            buffer_destroy(responseBuffer);
            log_info(kernelLogger, "MEM_WRITE <Carpincho %d>: Memoria->Carpincho exitosa", pcb->pid);
        } else {
            recv_empty_buffer(kernelCfg->MEMORIA_SOCKET);
            send_empty_buffer(FAIL, *(pcb->socket));
            log_error(kernelLogger, "MEM_WRITE <Carpincho %d>: Memoria->Carpincho fallida", pcb->pid);
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
        buffer_send(buffer, OK_CONTINUE, *(pcb->socket));
        free(contenidoAEscribir_Stub);
        log_info(kernelLogger, "MEM_WRITE <Carpincho %d>: Se escriben %d Bytes de dirección lógica %d", pcb->pid, cantidadBytesAEscribir_Stub, matePointerAEscribir_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_memread_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    pthread_mutex_lock(&mutexMemSocket);
    if(es_deploy_mode()) {
        buffer_send(buffer, MEM_READ, kernelCfg->MEMORIA_SOCKET);
        log_info(kernelLogger, "MEM_READ <Carpincho %d>: Carpincho->Memoria exitosa", pcb->pid);

        uint32_t response = get_op_code(kernelCfg->MEMORIA_SOCKET);

        if(response == OK_CONTINUE) {
            t_buffer* responseBuffer = buffer_create();
            get_buffer(kernelCfg->MEMORIA_SOCKET, responseBuffer);
            buffer_send(responseBuffer, OK_CONTINUE, *(pcb->socket));
            buffer_destroy(responseBuffer);
            log_info(kernelLogger, "MEM_READ <Carpincho %d>: Memoria->Carpincho exitosa", pcb->pid);
        } else {
            recv_empty_buffer(kernelCfg->MEMORIA_SOCKET);
            send_empty_buffer(FAIL, *(pcb->socket));
            log_error(kernelLogger, "MEM_READ <Carpincho %d>: Memoria->Carpincho fallida", pcb->pid);
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
        buffer_send(buffer, OK_CONTINUE, *(pcb->socket));
        log_info(kernelLogger, "MEM_READ <Carpincho %d>: Se leen %d Bytes de dirección lógica %d", pcb->pid, cantidadBytesALeer_Stub, matePointerALeer_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_memfree_a_memoria(t_pcb* pcb, t_buffer* buffer) {
    pthread_mutex_lock(&mutexMemSocket);
    if(es_deploy_mode()) {
        buffer_send(buffer, MEM_FREE, kernelCfg->MEMORIA_SOCKET);
        log_info(kernelLogger, "MEM_FREE <Carpincho %d>: Carpincho->Memoria exitosa", pcb->pid);

        uint32_t response = get_op_code(kernelCfg->MEMORIA_SOCKET);
        recv_empty_buffer(kernelCfg->MEMORIA_SOCKET);

        if(response == OK_CONTINUE) {
            send_empty_buffer(OK_CONTINUE, *(pcb->socket));
            log_info(kernelLogger, "MEM_FREE <Carpincho %d>: Memoria->Carpincho exitosa", pcb->pid);
        } else {
            send_empty_buffer(FAIL, *(pcb->socket));
            log_error(kernelLogger, "MEM_FREE <Carpincho %d>: Memoria->Carpincho fallida", pcb->pid);
        }
    } else {
        int32_t matePointerALiberar_Stub;
        uint32_t pid;
        buffer_unpack(buffer, &pid, sizeof(pid));
        buffer_unpack(buffer, &matePointerALiberar_Stub, sizeof(matePointerALiberar_Stub));
        send_empty_buffer(OK_CONTINUE, *(pcb->socket));
        log_info(kernelLogger, "MEM_FREE <Carpincho %d>: Se libera espacio de memoria de dirección lógica %d", pcb->pid, matePointerALiberar_Stub);
    }
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_mate_close_a_memoria(t_pcb* pcb) {
    pthread_mutex_lock(&mutexMemSocket);
    if(es_deploy_mode()) {
        t_buffer* closeBuffer = buffer_create();
        buffer_pack(closeBuffer, &(pcb->pid), sizeof(pcb->pid));
        buffer_send(closeBuffer, MATE_CLOSE, kernelCfg->MEMORIA_SOCKET);

        uint32_t response = get_op_code(kernelCfg->MEMORIA_SOCKET);

        if(response == OK_CONTINUE) {
            recv_empty_buffer(kernelCfg->MEMORIA_SOCKET);
        } else {
            log_error(kernelLogger, "EXIT: Error al intentar finalizar Carpincho ID %d de Memoria", pcb->pid);
            exit(0);
        }
    }
    log_info(kernelLogger, "EXIT: Se finaliza Carpincho ID %d", pcb->pid);
    pthread_mutex_unlock(&mutexMemSocket);
}

void enviar_suspension_de_carpincho_a_memoria(t_pcb* pcb) {
    if(es_deploy_mode()) {
        pthread_mutex_lock(&mutexMemSocket);
        t_buffer* closeBuffer = buffer_create();
        buffer_pack(closeBuffer, &(pcb->pid), sizeof(pcb->pid));
        buffer_send(closeBuffer, SUSPEND_CARPINCHO, kernelCfg->MEMORIA_SOCKET);

        uint32_t response = get_op_code(kernelCfg->MEMORIA_SOCKET);

        if(response == OK_CONTINUE) {
            recv_empty_buffer(kernelCfg->MEMORIA_SOCKET);
        } else {
            log_error(kernelLogger, "Suspensión: Error al intentar suspender un Carpincho ID %d de Memoria", pcb->pid);
            exit(0);
        }
    }
    log_info(kernelLogger, "Suspensión: Se suspende Carpincho ID %d", pcb->pid);
    pthread_mutex_unlock(&mutexMemSocket);
}
