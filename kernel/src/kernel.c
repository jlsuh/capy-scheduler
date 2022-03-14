#include "kernel.h"

#include <commons/log.h>
#include <commons/string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>

#include "common_flags.h"
#include "connections.h"
#include "kernel_config.h"
#include "scheduler.h"
#include "stream.h"

#define KERNEL_LOG_DEST "bin/kernel.log"
#define KERNEL_MODULE_NAME "Kernel"

t_log* kernelLogger;
t_kernel_config* kernelCfg;

static noreturn void __aceptar_conexiones_kernel(int socketEscucha, struct sockaddr cliente, socklen_t);
static void __crear_hilo_handler_conexion_entrante(int* socket);

int main(int argc, char* argv[]) {
    bool activeConsole = true;
    kernelLogger = log_create(KERNEL_LOG_DEST, KERNEL_MODULE_NAME, activeConsole, LOG_LEVEL_INFO);
    kernelCfg = kernel_config_create(argv[1]);

    int socketEscucha = iniciar_servidor(kernel_config_get_mem_ip(kernelCfg), kernel_config_get_mem_port(kernelCfg));

    log_info(kernelLogger, "Kernel: Modo de ejecución %s", EXEC_MODE);

    struct sockaddr cliente;
    socklen_t len = sizeof(cliente);

    if (string_equals_ignore_case(EXEC_MODE, "DEPLOY_MODE")) {
        log_info(kernelLogger, "Kernel: Esperando conexión entrante de Módulo Memoria...");
        kernel_config_set_mem_socket(kernelCfg, accept(socketEscucha, &cliente, &len));

        int32_t memSocket = kernel_config_get_mem_socket(kernelCfg);
        if (-1 == memSocket) {
            log_error(kernelLogger, "Kernel: Error al intentar establecer conexión con Módulo Memoria");
            liberar_modulo_kernel(kernelLogger, kernelCfg);
            exit(0);
        }

        uint32_t response = stream_recv_op_code(memSocket);
        stream_recv_empty_buffer(memSocket);

        if (MEMORIA == response) {
            stream_send_empty_buffer(OK_CONTINUE, memSocket);
            log_info(kernelLogger, "Kernel: Memoria establece conexión con Kernel en socket ID %d", memSocket);
        } else {
            stream_send_empty_buffer(FAIL, memSocket); /* Es entidad conectante pero no es Memoria */
            log_error(kernelLogger, "Kernel: Error al intentar establecer conexión con Módulo Memoria");
            liberar_modulo_kernel(kernelLogger, kernelCfg);
            exit(0);
        }
    }

    iniciar_planificacion();
    __aceptar_conexiones_kernel(socketEscucha, cliente, len);

    liberar_modulo_kernel(kernelLogger, kernelCfg);

    return EXIT_SUCCESS;
}

static noreturn void __aceptar_conexiones_kernel(int socketEscucha, struct sockaddr cliente, socklen_t len) {
    log_info(kernelLogger, "Kernel: A la escucha de nuevas conexiones en puerto %d", socketEscucha);
    int* socketCliente;
    for (;;) {
        socketCliente = malloc(sizeof(*socketCliente));
        *socketCliente = accept(socketEscucha, &cliente, &len);
        if (*socketCliente > 0) {
            __crear_hilo_handler_conexion_entrante(socketCliente);
        } else {
            log_error(kernelLogger, "Kernel: Error al aceptar conexión: %s", strerror(errno));
        }
    }
}

static void __crear_hilo_handler_conexion_entrante(int* socket) {
    pthread_t threadSuscripcion;
    pthread_create(&threadSuscripcion, NULL, encolar_en_new_nuevo_carpincho_entrante, (void*)socket);
    pthread_detach(threadSuscripcion);
    return;
}
