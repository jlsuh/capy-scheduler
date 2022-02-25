#include "module_config.h"

#include <stdlib.h>

int config_init(void* moduleConfig, char* pathToConfig, t_log* logger,
                void (*config_initializer)(void* moduleConfig, t_config* tempConfig)) {
    t_config* tempConfig = config_create(pathToConfig);
    if (tempConfig == NULL) {
        log_error(logger, "Path \"%s\" no encontrado", pathToConfig);
        return EXIT_FAILURE;
    }
    config_initializer(moduleConfig, tempConfig);
    log_info(logger, "Inicialización de campos correcta");
    config_destroy(tempConfig);
    return EXIT_SUCCESS;
}
