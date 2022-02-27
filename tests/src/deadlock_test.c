#include "deadlock_test.h"

#include <CUnit/Basic.h>
#include <commons/string.h>
#include <stdlib.h>
#include <sys/param.h>

#include "common_utils.h"
#include "duplicated_logic_allocator.h"
#include "kernel_structs.h"

typedef enum {
    PCB_LIST,
    SEM_LIST
} t_dimention_type;

static t_cola_recursos* colaSems;
static t_recurso_sem* sem1;
static t_recurso_sem* sem2;
static t_recurso_sem* sem3;
static t_recurso_sem* sem4;
static t_recurso_sem* sem5;
static t_recurso_sem* sem6;
static t_recurso_sem* sem7;
static t_pcb* pcb1;
static t_pcb* pcb2;
static t_pcb* pcb3;
static t_pcb* pcb4;
static t_pcb* pcb5;
static t_pcb* pcb6;
static uint32_t* pid1;
static uint32_t* pid2;
static uint32_t* pid3;
static uint32_t* pid4;
static uint32_t* pid5;
static uint32_t* pid6;

static bool __deadlock_retiene_instancias_del_semaforo(t_pcb* pcb, int32_t instanciasRetenidasExpected, t_recurso_sem* sem) {
    void* instanciasRetenidas = dictionary_get(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre);
    int32_t intanciasRetenidasSem = *(int32_t*)instanciasRetenidas;
    return instanciasRetenidas != NULL && intanciasRetenidasSem == instanciasRetenidasExpected;
}

// @Before
void test_deadlock_setup(void) {
    pid1 = malloc(sizeof(*pid1));
    pid2 = malloc(sizeof(*pid2));
    pid3 = malloc(sizeof(*pid3));
    pid4 = malloc(sizeof(*pid4));
    pid5 = malloc(sizeof(*pid5));
    pid6 = malloc(sizeof(*pid6));

    *pid1 = 1;
    *pid2 = 2;
    *pid3 = 3;
    *pid4 = 4;
    *pid5 = 5;
    *pid6 = 6;

    sem1 = recurso_sem_create("SEM1", 1);
    sem2 = recurso_sem_create("SEM2", 0);
    sem3 = recurso_sem_create("SEM3", 0);
    sem4 = recurso_sem_create("SEM4", 1);
    sem5 = recurso_sem_create("SEM5", 4);
    sem6 = recurso_sem_create("SEM6", 1);
    sem7 = recurso_sem_create("SEM7", 0);

    pcb1 = pcb_create(pid1, "SJF");
    pcb2 = pcb_create(pid2, "SJF");
    pcb3 = pcb_create(pid3, "SJF");
    pcb4 = pcb_create(pid4, "SJF");
    pcb5 = pcb_create(pid5, "SJF");
    pcb6 = pcb_create(pid6, "SJF");

    colaSems = cola_recursos_create();
    list_add(colaSems->listaRecursos, sem1);
    list_add(colaSems->listaRecursos, sem2);
    list_add(colaSems->listaRecursos, sem3);
    list_add(colaSems->listaRecursos, sem4);
    list_add(colaSems->listaRecursos, sem5);
    list_add(colaSems->listaRecursos, sem6);
    list_add(colaSems->listaRecursos, sem7);
}

// @After
void test_deadlock_tear_down(void) {
    recurso_sem_destroy(sem1);
    recurso_sem_destroy(sem2);
    recurso_sem_destroy(sem3);
    recurso_sem_destroy(sem4);
    recurso_sem_destroy(sem5);
    recurso_sem_destroy(sem6);
    recurso_sem_destroy(sem7);

    pcb1->algoritmo_destroy(pcb1);
    pcb2->algoritmo_destroy(pcb2);
    pcb3->algoritmo_destroy(pcb3);
    pcb4->algoritmo_destroy(pcb4);
    pcb5->algoritmo_destroy(pcb5);
    pcb6->algoritmo_destroy(pcb6);

    cola_recursos_destroy(colaSems);
}

void test_sem_create(void) {
    CU_ASSERT_EQUAL(1, sem1->valorActual);
    CU_ASSERT_EQUAL(sem1->valorActual, sem1->valorInicial);
}

void test_pcb_create(void) {
    CU_ASSERT_PTR_NULL(pcb1->deadlockInfo->esperaEnSemaforo);
    CU_ASSERT_TRUE(dictionary_is_empty(pcb2->deadlockInfo->semaforosQueRetiene));
}

void test_un_proceso_retiene_una_instancia_del_semaforo_en_caso_de_no_bloquearse(void) {
    bool esBloqueante = kernel_sem_wait(sem1, pcb1);
    CU_ASSERT_FALSE(esBloqueante);
    CU_ASSERT_PTR_NULL(pcb1->deadlockInfo->esperaEnSemaforo);

    CU_ASSERT_TRUE(__deadlock_retiene_instancias_del_semaforo(pcb1, 1, sem1));
}

void test_un_proceso_no_retiene_una_instancia_del_semaforo_en_caso_de_bloquearse(void) {
    bool esBloqueante = kernel_sem_wait(sem2, pcb1);
    CU_ASSERT_TRUE(esBloqueante);
    CU_ASSERT_STRING_EQUAL(pcb1->deadlockInfo->esperaEnSemaforo->nombre, "SEM2");

    void* instanciasRetenidas = dictionary_get(pcb1->deadlockInfo->semaforosQueRetiene, sem1->nombre);
    CU_ASSERT_PTR_NULL(instanciasRetenidas);
}

void test_un_proceso_retiene_una_instancia_de_un_semaforo_bloqueandose_luego_en_el_mismo(void) {
    bool esBloqueante = kernel_sem_wait(sem1, pcb1);
    CU_ASSERT_FALSE(esBloqueante);
    CU_ASSERT_PTR_NULL(pcb1->deadlockInfo->esperaEnSemaforo);

    CU_ASSERT_TRUE(__deadlock_retiene_instancias_del_semaforo(pcb1, 1, sem1));

    esBloqueante = kernel_sem_wait(sem1, pcb1);
    CU_ASSERT_TRUE(esBloqueante);
    CU_ASSERT_STRING_EQUAL(pcb1->deadlockInfo->esperaEnSemaforo->nombre, "SEM1");

    CU_ASSERT_TRUE(__deadlock_retiene_instancias_del_semaforo(pcb1, 1, sem1));
}

void test_un_proceso_retiene_una_instancia_de_un_semaforo_pero_se_bloquea_en_otro_semaforo(void) {
    bool esBloqueante = kernel_sem_wait(sem1, pcb1);
    CU_ASSERT_FALSE(esBloqueante);
    CU_ASSERT_PTR_NULL(pcb1->deadlockInfo->esperaEnSemaforo);

    CU_ASSERT_TRUE(__deadlock_retiene_instancias_del_semaforo(pcb1, 1, sem1));

    esBloqueante = kernel_sem_wait(sem2, pcb1);
    CU_ASSERT_TRUE(esBloqueante);
    CU_ASSERT_STRING_EQUAL(pcb1->deadlockInfo->esperaEnSemaforo->nombre, "SEM2");

    void* instanciasRetenidas = dictionary_get(pcb1->deadlockInfo->semaforosQueRetiene, sem2->nombre);
    CU_ASSERT_PTR_NULL(instanciasRetenidas);
}

void test_un_proceso_retiene_dos_instancias_de_un_semaforo(void) {
    kernel_sem_wait(sem5, pcb5);
    kernel_sem_wait(sem5, pcb5);

    CU_ASSERT_TRUE(__deadlock_retiene_instancias_del_semaforo(pcb5, 2, sem5));
}

void test_un_proceso_al_hacer_un_post_deja_de_tener_una_intancia_que_retenia(void) {
    kernel_sem_wait(sem5, pcb5);

    CU_ASSERT_TRUE(dictionary_has_key(pcb5->deadlockInfo->semaforosQueRetiene, sem5->nombre));

    kernel_sem_post(sem5, pcb5);

    CU_ASSERT_FALSE(dictionary_has_key(pcb5->deadlockInfo->semaforosQueRetiene, sem5->nombre));
}

void test_un_proceso_al_hacer_un_post_de_un_semaforo_que_no_retenia_sigue_sin_retener_ninguna_instancia(void) {
    kernel_sem_post(sem1, pcb5);

    CU_ASSERT_FALSE(dictionary_has_key(pcb5->deadlockInfo->semaforosQueRetiene, sem5->nombre));
}

static bool pcb_espera_algun_semaforo(void* pcbVoid) {
    t_pcb* pcb = (t_pcb*)pcbVoid;
    return pcb->deadlockInfo->esperaEnSemaforo != NULL;
}

static bool __espera_al_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    if (pcb_espera_algun_semaforo(pcb) && strcmp(pcb->deadlockInfo->esperaEnSemaforo->nombre, sem->nombre) == 0) {
        return true;
    }
    return false;
}

static bool esta_reteniendo_instancias_del_semaforo(t_pcb* pcb, t_recurso_sem* sem) {
    return dictionary_has_key(pcb->deadlockInfo->semaforosQueRetiene, sem->nombre);
}

static bool __eliminar_celdas_nulas_consecutivas(t_list* firstToIterateList, t_dimention_type firstToIterateType, t_list* pivotList) {
    /* Eliminar:
    - Procesos que no poseen retención o espera
    - Recursos que nadie retiene o espera
    */
    int initInnerSize = list_size(firstToIterateList);
    int initPivotSize = list_size(pivotList);
    for (int i = 0; i < list_size(pivotList); i++) {
        t_recurso_sem* sem = NULL;
        t_pcb* pcb = NULL;
        bool espera = false;
        bool retencion = false;
        bool existenCeldasNoNulas = false;
        for (int j = 0; j < list_size(firstToIterateList); j++) {
            if (firstToIterateType == PCB_LIST) {
                sem = list_get(pivotList, i);
                pcb = list_get(firstToIterateList, j);
            } else if (firstToIterateType == SEM_LIST) {
                sem = list_get(firstToIterateList, j);
                pcb = list_get(pivotList, i);
            }
            if (!espera) {
                espera = __espera_al_semaforo(pcb, sem);
            }
            if (!retencion) {
                retencion = esta_reteniendo_instancias_del_semaforo(pcb, sem);
            }
            if (espera && retencion) {
                existenCeldasNoNulas = true;
                break;
            }
        }
        if (!existenCeldasNoNulas) {
            list_remove(pivotList, i);
            i--;
        }
    }
    return list_size(firstToIterateList) < initInnerSize || list_size(pivotList) < initPivotSize;
}

static bool __reducir_matrices_de_deteccion(t_list* pcbsDeadlock, t_list* semsDeadlock) {
    bool seEliminaronColumnas = true;
    bool seEliminaronFilas = true;
    while (seEliminaronColumnas || seEliminaronFilas) {
        seEliminaronColumnas = __eliminar_celdas_nulas_consecutivas(pcbsDeadlock, PCB_LIST, semsDeadlock);
        seEliminaronFilas = __eliminar_celdas_nulas_consecutivas(semsDeadlock, SEM_LIST, pcbsDeadlock);
    }
    return !(list_size(pcbsDeadlock) == 0 && list_size(semsDeadlock) == 0);
}

static void print_alloced_resources_matrix(t_list* pcbsDeadlock, t_list* semsDeadlock) {
    printf("\nMatriz de recursos asignados\n");
    for (int i = 0; i < list_size(pcbsDeadlock); i++) {
        t_pcb* pcb = list_get(pcbsDeadlock, i);
        for (int j = 0; j < list_size(semsDeadlock); j++) {
            t_recurso_sem* sem = list_get(semsDeadlock, j);
            t_dictionary* dict = pcb->deadlockInfo->semaforosQueRetiene;
            if (dictionary_has_key(dict, sem->nombre)) {
                printf("%d ", *(int*)dictionary_get(dict, sem->nombre));
            } else {
                printf("0 ");
            }
        }
        printf("\n");
    }
}

static void print_awaiting_resources_matrix(t_list* pcbsDeadlock, t_list* semsDeadlock) {
    printf("\nMatriz de peticiones pendientes\n");
    for (int i = 0; i < list_size(pcbsDeadlock); i++) {
        t_pcb* pcb = list_get(pcbsDeadlock, i);
        for (int j = 0; j < list_size(semsDeadlock); j++) {
            t_recurso_sem* sem = list_get(semsDeadlock, j);
            if (__espera_al_semaforo(pcb, sem)) {
                printf("1 ");
            } else {
                printf("0 ");
            }
        }
        printf("\n");
    }
}

void test_es_posible_eliminar_columnas_y_filas_nulas_1(void) {
    /* TODO: En la implementación real filtrar de blocked y susp/blocked los procesos que estén a la espera de un semáforo */
    t_list* pcbsBlocked = list_create();
    t_list* semsDeadlock = list_create();

    list_add_all(semsDeadlock, colaSems->listaRecursos);

    CU_ASSERT_FALSE(kernel_sem_wait(sem1, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(sem4, pcb4));
    CU_ASSERT_TRUE(kernel_sem_wait(sem4, pcb1));
    list_add(pcbsBlocked, pcb1);
    CU_ASSERT_TRUE(kernel_sem_wait(sem1, pcb4));
    list_add(pcbsBlocked, pcb4);
    CU_ASSERT_TRUE(kernel_sem_wait(sem2, pcb2));
    list_add(pcbsBlocked, pcb2);
    CU_ASSERT_TRUE(kernel_sem_wait(sem3, pcb3));
    list_add(pcbsBlocked, pcb3);
    CU_ASSERT_FALSE(kernel_sem_wait(sem5, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(sem5, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(sem6, pcb6));
    CU_ASSERT_TRUE(kernel_sem_wait(sem7, pcb6));
    list_add(pcbsBlocked, pcb6);

    /* Suponiendo que SEM5 no forma parte de los procesos bloqueados por un semáforo (sino que bloqueado esperando un dispositivo I/O por ejmplo) */
    t_list* pcbsDeadlock = list_filter(pcbsBlocked, pcb_espera_algun_semaforo);

    CU_ASSERT_EQUAL(list_size(pcbsDeadlock), 5);
    CU_ASSERT_EQUAL(list_size(semsDeadlock), 7);

    __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);

    CU_ASSERT_EQUAL(list_size(pcbsDeadlock), 2);
    CU_ASSERT_EQUAL(list_size(semsDeadlock), 2);

    list_destroy(pcbsBlocked);
    list_destroy(pcbsDeadlock);
    list_destroy(semsDeadlock);
}

void test_es_posible_eliminar_columnas_y_filas_nulas_2(void) {
    t_list* pcbsBlocked = list_create();
    t_list* semsDeadlock = list_create();

    list_add_all(semsDeadlock, colaSems->listaRecursos);

    CU_ASSERT_FALSE(kernel_sem_wait(sem1, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(sem4, pcb4));
    CU_ASSERT_TRUE(kernel_sem_wait(sem4, pcb1));
    list_add(pcbsBlocked, pcb1);
    CU_ASSERT_TRUE(kernel_sem_wait(sem1, pcb4));
    list_add(pcbsBlocked, pcb4);

    t_list* pcbsDeadlock = list_filter(pcbsBlocked, pcb_espera_algun_semaforo);

    CU_ASSERT_EQUAL(list_size(pcbsDeadlock), 2);
    CU_ASSERT_EQUAL(list_size(semsDeadlock), 7);

    __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);

    CU_ASSERT_EQUAL(list_size(pcbsDeadlock), 2);
    CU_ASSERT_EQUAL(list_size(semsDeadlock), 2);

    list_destroy(pcbsBlocked);
    list_destroy(pcbsDeadlock);
    list_destroy(semsDeadlock);
}

void test_es_posible_eliminar_columnas_y_filas_nulas_3(void) {
    t_list* pcbsBlocked = list_create();
    t_list* semsDeadlock = list_create();
    t_recurso_sem* R1 = recurso_sem_create("R1", 4);
    t_recurso_sem* R2 = recurso_sem_create("R2", 3);
    t_recurso_sem* R3 = recurso_sem_create("R3", 4);
    t_recurso_sem* R4 = recurso_sem_create("R4", 4);
    list_add(semsDeadlock, R1);
    list_add(semsDeadlock, R2);
    list_add(semsDeadlock, R3);
    list_add(semsDeadlock, R4);

    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb5));

    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb5));

    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb5));

    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb3));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb5));

    CU_ASSERT_TRUE(kernel_sem_wait(R2, pcb1));
    list_add(pcbsBlocked, pcb1);
    CU_ASSERT_TRUE(kernel_sem_wait(R4, pcb2));
    list_add(pcbsBlocked, pcb2);
    CU_ASSERT_TRUE(kernel_sem_wait(R3, pcb3));
    list_add(pcbsBlocked, pcb3);
    CU_ASSERT_TRUE(kernel_sem_wait(R1, pcb4));
    list_add(pcbsBlocked, pcb4);
    CU_ASSERT_TRUE(kernel_sem_wait(R1, pcb5));
    list_add(pcbsBlocked, pcb5);

    t_list* pcbsDeadlock = list_filter(pcbsBlocked, pcb_espera_algun_semaforo);

    CU_ASSERT_EQUAL(list_size(pcbsDeadlock), 5);
    CU_ASSERT_EQUAL(list_size(semsDeadlock), 4);

    __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);

    CU_ASSERT_EQUAL(list_size(pcbsDeadlock), 3);
    CU_ASSERT_EQUAL(list_size(semsDeadlock), 2);

    list_destroy(pcbsBlocked);
    list_destroy(pcbsDeadlock);
    list_destroy(semsDeadlock);
    recurso_sem_destroy(R1);
    recurso_sem_destroy(R2);
    recurso_sem_destroy(R3);
    recurso_sem_destroy(R4);
}

static bool es_este_semaforo(void* recursoSemVoid, void* nombreVoid) {
    t_recurso_sem* recursoSem = recursoSemVoid;
    char* nombre = nombreVoid;
    return string_equals_ignore_case(recursoSem->nombre, nombre);
}

static void __recuperarse_del_deadlock(t_list* pcbsDeadlock, t_list* semsDeadlock, t_list* semsList, t_list* pcbsBlocked /*, t_list* pcbsSusBlocked*/) {
    t_pcb* pcbDeMayorPID = list_get_maximum(pcbsDeadlock, pcb_maximum_pid);
    t_dictionary* semsQueRetienePCBDeMayorPID = pcbDeMayorPID->deadlockInfo->semaforosQueRetiene;
    for (int i = 0; i < list_size(semsList); i++) {
        t_recurso_sem* sem = list_get(semsList, i);
        if (esta_reteniendo_instancias_del_semaforo(pcbDeMayorPID, sem)) {
            t_recurso_sem* semEnListaDeadlock = list_find2(semsDeadlock, (void*)es_este_semaforo, (void*)(sem->nombre));
            bool esParteDelDedlock = semEnListaDeadlock != NULL;
            if (i == 0 && esParteDelDedlock) {
                // bool fueEliminado = eliminar_pcb_de_lista(pcbDeMayorPID, pcbsBlocked);
                eliminar_pcb_de_lista(pcbDeMayorPID, pcbsBlocked);
                /* if(!fueEliminado) {
                    eliminar_pcb_de_lista(pcbDeMayorPID, pcbsSusBlocked);
                } */
                eliminar_pcb_de_lista(pcbDeMayorPID, sem->colaPCBs->elements);
            }
            int cantInstRet = *(int*)dictionary_get(semsQueRetienePCBDeMayorPID, sem->nombre);
            while (cantInstRet > 0) {
                kernel_sem_post(sem, pcbDeMayorPID);
                cantInstRet--;
            }
        }
    }
    /* TODO: Luego hay que mandar a exit al pcb. Esto implica mandarle un buffer vacío con un handshake de finalización */
    /* El PCB todavía posee pcbDeMayorPID->deadlockInfo->esperaEnSemaforo != NULL, pero es irrelevante */
}

static void detectar_y_recuperarse_del_deadlock(t_list* pcbsBlocked, t_cola_recursos* semaforosDelSistema) {
    t_list* pcbsDeadlock = list_filter(pcbsBlocked, pcb_espera_algun_semaforo); /* TODO: Tomar mutex de pcbsBlocked & pcbsSusBlocked */
    t_list* semsDeadlock = list_create();
    list_add_all(semsDeadlock, semaforosDelSistema->listaRecursos);

    print_awaiting_resources_matrix(pcbsDeadlock, semsDeadlock);
    print_alloced_resources_matrix(pcbsDeadlock, semsDeadlock);

    bool existeDeadlock = __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);

    print_awaiting_resources_matrix(pcbsDeadlock, semsDeadlock);
    print_alloced_resources_matrix(pcbsDeadlock, semsDeadlock);

    while (existeDeadlock) {
        __recuperarse_del_deadlock(pcbsDeadlock, semsDeadlock, semaforosDelSistema->listaRecursos, pcbsBlocked);
        print_awaiting_resources_matrix(pcbsDeadlock, semsDeadlock);
        print_alloced_resources_matrix(pcbsDeadlock, semsDeadlock);
        existeDeadlock = __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);
        print_awaiting_resources_matrix(pcbsDeadlock, semsDeadlock);
        print_alloced_resources_matrix(pcbsDeadlock, semsDeadlock);
    }

    list_destroy(pcbsDeadlock);
    list_destroy(semsDeadlock);
}

void test_deteccion_y_recuperacion_del_deadlock_1(void) {
    t_list* pcbsBlocked = list_create();
    t_recurso_sem* R1 = recurso_sem_create("R1", 4);
    t_recurso_sem* R2 = recurso_sem_create("R2", 3);
    t_recurso_sem* R3 = recurso_sem_create("R3", 4);
    t_recurso_sem* R4 = recurso_sem_create("R4", 4);
    list_clean(colaSems->listaRecursos);
    list_add(colaSems->listaRecursos, R1);
    list_add(colaSems->listaRecursos, R2);
    list_add(colaSems->listaRecursos, R3);
    list_add(colaSems->listaRecursos, R4);

    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb5));

    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb5));

    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb5));

    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb3));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb5));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb5));

    CU_ASSERT_TRUE(kernel_sem_wait(R2, pcb1));
    list_add(pcbsBlocked, pcb1);
    CU_ASSERT_TRUE(kernel_sem_wait(R4, pcb2));
    list_add(pcbsBlocked, pcb2);
    CU_ASSERT_TRUE(kernel_sem_wait(R3, pcb3));
    list_add(pcbsBlocked, pcb3);
    CU_ASSERT_TRUE(kernel_sem_wait(R1, pcb4));
    list_add(pcbsBlocked, pcb4);
    CU_ASSERT_TRUE(kernel_sem_wait(R1, pcb5));
    list_add(pcbsBlocked, pcb5);

    detectar_y_recuperarse_del_deadlock(pcbsBlocked, colaSems);

    /* t_list* pcbsDeadlock = list_filter(pcbsBlocked, pcb_espera_algun_semaforo);
    bool existeDeadlock = __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);
    CU_ASSERT_TRUE(existeDeadlock);
    t_pcb* pcbDeMayorPID = list_get_maximum(pcbsDeadlock, pcb_maximum_pid);
    CU_ASSERT_EQUAL(*(pcbDeMayorPID->pid), 5);
    CU_ASSERT_EQUAL(dictionary_size(pcb5->deadlockInfo->semaforosQueRetiene), 4);
    __recuperarse_del_deadlock(pcbsDeadlock, semsDeadlock, colaSems->listaRecursos, pcbsBlocked);
    CU_ASSERT_EQUAL(dictionary_size(pcb5->deadlockInfo->semaforosQueRetiene), 0);
    existeDeadlock = __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);
    CU_ASSERT_FALSE(existeDeadlock); */

    list_destroy(pcbsBlocked);
    recurso_sem_destroy(R1);
    recurso_sem_destroy(R2);
    recurso_sem_destroy(R3);
    recurso_sem_destroy(R4);
}

void test_deteccion_y_recuperacion_del_deadlock_2(void) {
    t_list* pcbsBlocked = list_create();
    t_recurso_sem* R1 = recurso_sem_create("R1", 5);
    t_recurso_sem* R2 = recurso_sem_create("R2", 3);
    t_recurso_sem* R3 = recurso_sem_create("R3", 2);
    t_recurso_sem* R4 = recurso_sem_create("R4", 4);
    list_clean(colaSems->listaRecursos);
    list_add(colaSems->listaRecursos, R1);
    list_add(colaSems->listaRecursos, R2);
    list_add(colaSems->listaRecursos, R3);
    list_add(colaSems->listaRecursos, R4);

    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb2));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb2));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb2));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R1, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb1));
    CU_ASSERT_FALSE(kernel_sem_wait(R2, pcb3));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb2));
    CU_ASSERT_FALSE(kernel_sem_wait(R3, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb4));
    CU_ASSERT_FALSE(kernel_sem_wait(R4, pcb4));

    CU_ASSERT_TRUE(kernel_sem_wait(R2, pcb2));
    list_add(pcbsBlocked, pcb2);
    CU_ASSERT_TRUE(kernel_sem_wait(R3, pcb3));
    list_add(pcbsBlocked, pcb3);

    detectar_y_recuperarse_del_deadlock(pcbsBlocked, colaSems);

    /* t_list* pcbsDeadlock = list_filter(pcbsBlocked, pcb_espera_algun_semaforo);
    bool existeDeadlock = __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);
    CU_ASSERT_TRUE(existeDeadlock);
    t_pcb* pcbDeMayorPID = list_get_maximum(pcbsDeadlock, pcb_maximum_pid);
    CU_ASSERT_EQUAL(*(pcbDeMayorPID->pid), 3);
    CU_ASSERT_EQUAL(dictionary_size(pcb3->deadlockInfo->semaforosQueRetiene), 1);
    __recuperarse_del_deadlock(pcbsDeadlock, semsDeadlock, colaSems->listaRecursos, pcbsBlocked);
    CU_ASSERT_EQUAL(dictionary_size(pcb3->deadlockInfo->semaforosQueRetiene), 0);
    existeDeadlock = __reducir_matrices_de_deteccion(pcbsDeadlock, semsDeadlock);
    CU_ASSERT_FALSE(existeDeadlock); */

    list_destroy(pcbsBlocked);
    recurso_sem_destroy(R1);
    recurso_sem_destroy(R2);
    recurso_sem_destroy(R3);
    recurso_sem_destroy(R4);
}
