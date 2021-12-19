#include "kernel.h"

int main(int argc, char* argv[]) {
    kernelCfg = kernel_cfg_create();
    kernelLogger = log_create(KERNEL_LOG_DEST, KERNEL_MODULE_NAME, true, LOG_LEVEL_INFO);
    cargar_configuracion(KERNEL_MODULE_NAME, kernelCfg, KERNEL_CFG_PATH, kernelLogger, kernel_config_initialize);

    int socketEscucha = iniciar_servidor(kernelCfg->IP_MEMORIA, kernelCfg->PUERTO_MEMORIA);

    log_info(kernelLogger, "Kernel: Modo de ejecución %s", EXEC_MODE);

    struct sockaddr cliente;
    socklen_t len = sizeof(cliente);

    if(string_equals_ignore_case(EXEC_MODE, "DEPLOY_MODE")) {
        log_info(kernelLogger, "Kernel: Esperando conexión entrante de Módulo Memoria...");
        kernelCfg->MEMORIA_SOCKET = accept(socketEscucha, &cliente, &len);

        if(kernelCfg->MEMORIA_SOCKET == -1) {
            log_error(kernelLogger, "Kernel: Error al intentar establecer conexión con Módulo Memoria");
            liberar_modulo_kernel(kernelLogger, kernelCfg);
            exit(0);
        }

        uint32_t response = get_op_code(kernelCfg->MEMORIA_SOCKET);
        recv_empty_buffer(kernelCfg->MEMORIA_SOCKET);

        if(response == MEMORIA) {
            send_empty_buffer(OK_CONTINUE, kernelCfg->MEMORIA_SOCKET);
            log_info(kernelLogger, "Kernel: Memoria establece conexión con Kernel en socket ID %d", kernelCfg->MEMORIA_SOCKET);
        } else {
            send_empty_buffer(FAIL, kernelCfg->MEMORIA_SOCKET); /* En este caso kernelCfg->MEMORIA_SOCKET sería una entidad que se conectó pero que en realidad no es Memoria */
            log_error(kernelLogger, "Kernel: Error al intentar establecer conexión con Módulo Memoria");
            liberar_modulo_kernel(kernelLogger, kernelCfg);
            exit(0);
        }
    }

    iniciar_planificacion();
    aceptar_conexiones_kernel(socketEscucha, cliente, len);

    liberar_modulo_kernel(kernelLogger, kernelCfg);

    return EXIT_SUCCESS;
}

void aceptar_conexiones_kernel(int socketEscucha, struct sockaddr cliente, socklen_t len) {
    log_info(kernelLogger, "Kernel: A la escucha de nuevas conexiones en puerto %d", socketEscucha);
    int* socketCliente;
    for(;;) {
        socketCliente = malloc(sizeof(int));
        *socketCliente = accept(socketEscucha, &cliente, &len);
        if(*socketCliente > 0) {
            crear_hilo_handler_conexion_entrante(socketCliente);
        } else {
            log_error(kernelLogger, "Kernel: Error al aceptar conexión: %s", strerror(errno));
        }
    }
}

void crear_hilo_handler_conexion_entrante(int* socket) {
    pthread_t threadSuscripcion;
    pthread_create(&threadSuscripcion, NULL, encolar_en_new_nuevo_carpincho_entrante, (void *) socket);
    pthread_detach(threadSuscripcion);
    return;
}
