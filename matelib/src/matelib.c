#include <commons/config.h>
#include <matelib.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "common_flags.h"
#include "connections.h"
#include "stream.h"

#define MATELIB_CFG_PATH "cfg/matelib_config.cfg"
#define MATE_LOG_PATH "bin/matelib_log.log"

//------------------ Static Functions ---------------------/
static void buffer_pack_mate_call_io(t_buffer* buffer, mate_io_resource io, char* msg);
static void buffer_pack_sem_init(t_buffer* buffer, mate_sem_name sem, uint32_t value);
static void liberar_recursos(mate_inner_structure* mateInner);

//------------------ General Functions ---------------------/
int mate_init(mate_instance* libRef, char* configPath) {
    mate_inner_structure* mateInner = malloc(sizeof(*mateInner));
    int resultado = -1;

    t_config* config = config_create(configPath);
    mateInner->ip = strdup(config_get_string_value(config, "IP"));
    mateInner->puerto = strdup(config_get_string_value(config, "PUERTO"));
    mateInner->mateLogger = log_create(MATE_LOG_PATH, "Carpincho", 1, LOG_LEVEL_INFO);
    config_destroy(config);

    int32_t conexion = conectar_a_servidor(mateInner->ip, mateInner->puerto);
    if (-1 == conexion) {
        log_error(mateInner->mateLogger, "Carpincho: No se pudo establecer conexión con Backend. Valor conexión %d", conexion);
        liberar_recursos(mateInner);
        return resultado;
    }
    mateInner->socketConexion = conexion;

    stream_send_empty_buffer(MATE_INIT, mateInner->socketConexion);

    t_buffer* buffer = buffer_create();
    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_buffer(mateInner->socketConexion, buffer);

    if (OK_CONTINUE == response) {
        resultado = 1;
        libRef->group_info = mateInner;
        uint32_t pid;
        buffer_unpack(buffer, &pid, sizeof(pid));
        mateInner->pid = pid;
        buffer_destroy(buffer);
        log_info(mateInner->mateLogger, "Carpincho %d: Establece conexión con Backend en socket ID %d", mateInner->pid, mateInner->socketConexion);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: No se pudo establecer conexión con Backend", mateInner->pid);
        liberar_recursos(mateInner);
        buffer_destroy(buffer);
        exit(0);
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

int mate_call_io(mate_instance* libRef, mate_io_resource io, void* msg) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferCallIO = buffer_create();
    buffer_pack_mate_call_io(bufferCallIO, io, (char*)msg);
    stream_send_buffer(bufferCallIO, CALL_IO, mateInner->socketConexion);
    buffer_destroy(bufferCallIO);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        log_info(mateInner->mateLogger, "Carpincho %d: I/O con mensaje \"%s\" exitosa en dispositivo <%s>", mateInner->pid, (char*)msg, io);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: No se pudo realizar la I/O en dispositivo <%s>", mateInner->pid, io);
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

int mate_close(mate_instance* libRef) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;

    stream_send_empty_buffer(MATE_CLOSE, mateInner->socketConexion);
    int resultado = 1;
    log_info(mateInner->mateLogger, "Carpincho %d: Finalización del carpincho", mateInner->pid);
    liberar_recursos(mateInner);

    return resultado;
}

int mate_sem_init(mate_instance* libRef, mate_sem_name sem, unsigned int value) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferSemInit = buffer_create();
    uint32_t value32 = value;
    buffer_pack_sem_init(bufferSemInit, sem, value32);
    stream_send_buffer(bufferSemInit, SEM_INIT, mateInner->socketConexion);
    buffer_destroy(bufferSemInit);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        log_info(mateInner->mateLogger, "Carpincho %d: Inicialización de semáforo %s con valor %d exitosa", mateInner->pid, sem, value);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: No se pudo inicializar el semáforo %s con valor %d", mateInner->pid, sem, value);
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

int mate_sem_wait(mate_instance* libRef, mate_sem_name sem) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferSemWait = buffer_create();
    buffer_pack_string(bufferSemWait, sem);

    stream_send_buffer(bufferSemWait, SEM_WAIT, mateInner->socketConexion);
    buffer_destroy(bufferSemWait);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        log_info(mateInner->mateLogger, "Carpincho %d: Wait al semáforo %s exitosa", mateInner->pid, sem);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: No se pudo hacer el Wait sobre el semáforo %s", mateInner->pid, sem);
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

int mate_sem_post(mate_instance* libRef, mate_sem_name sem) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferSemPost = buffer_create();
    buffer_pack_string(bufferSemPost, sem);
    stream_send_buffer(bufferSemPost, SEM_POST, mateInner->socketConexion);
    buffer_destroy(bufferSemPost);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        log_info(mateInner->mateLogger, "Carpincho %d: Post al semáforo %s exitosa", mateInner->pid, sem);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: No se pudo hacer el Post sobre el semáforo %s", mateInner->pid, sem);
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

int mate_sem_destroy(mate_instance* libRef, mate_sem_name sem) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferSemDestroy = buffer_create();
    buffer_pack_string(bufferSemDestroy, sem);
    stream_send_buffer(bufferSemDestroy, SEM_DESTROY, mateInner->socketConexion);
    buffer_destroy(bufferSemDestroy);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        log_info(mateInner->mateLogger, "Carpincho %d: Destrucción del semáforo %s exitosa", mateInner->pid, sem);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: No se pudo destruir el semáforo %s", mateInner->pid, sem);
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

mate_pointer mate_memalloc(mate_instance* libRef, int size) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;

    t_buffer* bufferMemAlloc = buffer_create();
    uint32_t size32 = size;
    buffer_pack(bufferMemAlloc, &(mateInner->pid), sizeof(mateInner->pid));
    buffer_pack(bufferMemAlloc, &size32, sizeof(size32));
    stream_send_buffer(bufferMemAlloc, MEM_ALLOC, mateInner->socketConexion);
    buffer_destroy(bufferMemAlloc);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    mate_pointer direccionAllocada;

    if (OK_CONTINUE == response) {
        bufferMemAlloc = buffer_create();
        stream_recv_buffer(mateInner->socketConexion, bufferMemAlloc);
        buffer_unpack(bufferMemAlloc, &direccionAllocada, sizeof(direccionAllocada));
        buffer_destroy(bufferMemAlloc);
        log_info(mateInner->mateLogger, "Carpincho %d: Alloc de %d Bytes en memoria de dirección lógica %d exitosa", mateInner->pid, size, direccionAllocada);
    } else {
        stream_recv_empty_buffer(mateInner->socketConexion);
        log_error(mateInner->mateLogger, "Carpincho %d: Denegación de alloc de %d Bytes en memoria", mateInner->pid, size);
        return -1;
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return direccionAllocada;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return direccionAllocada;
    }

    return -1;
}

int mate_memfree(mate_instance* libRef, mate_pointer addr) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferMemFree = buffer_create();
    buffer_pack(bufferMemFree, &(mateInner->pid), sizeof(mateInner->pid));
    buffer_pack(bufferMemFree, &addr, sizeof(addr));
    stream_send_buffer(bufferMemFree, MEM_FREE, mateInner->socketConexion);
    buffer_destroy(bufferMemFree);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        log_info(mateInner->mateLogger, "Carpincho %d: Liberación de memoria de dirección lógica %d exitosa", mateInner->pid, addr);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: Denegación de liberación de memoria de dirección lógica %d", mateInner->pid, addr);
        return MATE_FREE_FAULT;
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

int mate_memread(mate_instance* libRef, mate_pointer origin, void* dest, int size) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferMemRead = buffer_create();
    uint32_t sizeUint32 = size;
    buffer_pack(bufferMemRead, &(mateInner->pid), sizeof(mateInner->pid));
    buffer_pack(bufferMemRead, &origin, sizeof(origin));
    buffer_pack(bufferMemRead, &sizeUint32, sizeof(sizeUint32));
    stream_send_buffer(bufferMemRead, MEM_READ, mateInner->socketConexion);
    buffer_destroy(bufferMemRead);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        bufferMemRead = buffer_create();
        stream_recv_buffer(mateInner->socketConexion, bufferMemRead);
        memcpy(dest, bufferMemRead->stream, size);
        buffer_destroy(bufferMemRead);
        log_info(mateInner->mateLogger, "Carpincho %d: Lectura de %d Bytes en memoria de dirección lógica %d exitosa", mateInner->pid, size, origin);
    } else {
        stream_recv_empty_buffer(mateInner->socketConexion);
        log_error(mateInner->mateLogger, "Carpincho %d: Denegación de lectura en dirección lógica %d", mateInner->pid, origin);
        return MATE_READ_FAULT;
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

int mate_memwrite(mate_instance* libRef, void* origin, mate_pointer dest, int size) {
    mate_inner_structure* mateInner = (mate_inner_structure*)libRef->group_info;
    int resultado = -1;

    t_buffer* bufferMemWrite = buffer_create();
    uint32_t sizeUint32 = size;
    buffer_pack(bufferMemWrite, &(mateInner->pid), sizeof(mateInner->pid));
    buffer_pack(bufferMemWrite, &dest, sizeof(dest));
    buffer_pack(bufferMemWrite, &sizeUint32, sizeof(sizeUint32));
    buffer_pack(bufferMemWrite, origin, size);
    stream_send_buffer(bufferMemWrite, MEM_WRITE, mateInner->socketConexion);
    buffer_destroy(bufferMemWrite);

    uint32_t response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        resultado = 1;
        log_info(mateInner->mateLogger, "Carpincho %d: Escritura de %d Bytes en memoria de dirección lógica %d exitosa", mateInner->pid, size, dest);
    } else {
        log_error(mateInner->mateLogger, "Carpincho %d: Denegación de escritura en dirección lógica %d", mateInner->pid, size, dest);
        return MATE_WRITE_FAULT;
    }

    response = stream_recv_op_code(mateInner->socketConexion);
    stream_recv_empty_buffer(mateInner->socketConexion);

    if (OK_CONTINUE == response) {
        return resultado;
    } else if (DEADLOCK_END == response) {
        log_info(mateInner->mateLogger, "Carpincho %d: Finalización abrupta del carpincho por resolución de Deadlocks", mateInner->pid);
        liberar_recursos(mateInner);
        pthread_exit(NULL);
    } else if (OK_FINISH == response) {
        return resultado;
    }

    return -1;
}

static void liberar_recursos(mate_inner_structure* mateInner) {
    close(mateInner->socketConexion);
    free(mateInner->ip);
    free(mateInner->puerto);
    log_destroy(mateInner->mateLogger);
    free(mateInner);
}

static void buffer_pack_sem_init(t_buffer* buffer, mate_sem_name sem, uint32_t value) {
    buffer_pack_string(buffer, sem);
    buffer_pack(buffer, &value, sizeof(value));
}

static void buffer_pack_mate_call_io(t_buffer* buffer, mate_io_resource io, char* msg) {
    buffer_pack_string(buffer, io);
    buffer_pack_string(buffer, msg);
}
