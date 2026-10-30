#include "planificador.h"

#include <limits.h> //define los limites para que no suceda desbordamientos
#include <stdint.h> //define tipos de datos de tamaño fijo
#include <stdlib.h> //define funciones de memoria dinámica y ordenamiento como malloc free y el qsort

/* ============================================================
 * ESTRUCTURAS AUXILIARES INTERNAS DE FCFS
 * ============================================================ */
typedef struct { // Empaqueta estas variables para usarlas juntas.
    size_t indice_original;  // Guarda el "número de asiento" inicial (sin negativos) para no perder el orden al organizar.
    int64_t llegada; // Caja grande (64 bits) para tiempos largos; evita desbordamientos del servidor.
    int32_t prioridad; // Caja pequeña (32 bits) para números cortos; ahorra memoria.
} proceso_ordenado_fcfs_t;// La etiqueta o nombre oficial que le damos a este paquete.

/* ============================================================
 * CRITERIOS DE ORDENAMIENTO Y DESEMPATE
 * ============================================================
 * FCFS ordena primero por llegada. La prioridad solo desempata
 * llegadas iguales; el indice original resuelve el ultimo empate.
 */
static int comparar_procesos_fcfs(const void *izquierda, const void *derecha)
{
    // Convertimos los datos "crudos" al paquete (struct) que creamos antes.
    const proceso_ordenado_fcfs_t *proceso_izquierdo = izquierda;
    const proceso_ordenado_fcfs_t *proceso_derecho = derecha;

    if (proceso_izquierdo->llegada < proceso_derecho->llegada) {
        return -1;
    }
    if (proceso_izquierdo->llegada > proceso_derecho->llegada) {
        return 1;
    }
    if (proceso_izquierdo->prioridad < proceso_derecho->prioridad) {
        return -1;
    }
    if (proceso_izquierdo->prioridad > proceso_derecho->prioridad) {
        return 1;
    }
    if (proceso_izquierdo->indice_original < proceso_derecho->indice_original) {
        return -1;
    }
    if (proceso_izquierdo->indice_original > proceso_derecho->indice_original) {
        return 1;
    }
    return 0;
}

/* ============================================================
 * FUNCIONES AUXILIARES DE FCFS
 * ============================================================ */
static planificador_estado_t contar_segmentos_necesarios_fcfs(
    const proceso_t *procesos,
    const proceso_ordenado_fcfs_t *procesos_ordenados,
    size_t cantidad_procesos,
    size_t *cantidad_segmentos_necesaria)
{
    int64_t reloj_cpu = 0;        // El reloj de la CPU arranca en 0.
    size_t total_segmentos = 0;   // Contador para saber cuántos bloques necesitaremos en total.

    for (size_t turno = 0; turno < cantidad_procesos; turno++) {
        // Obtenemos el proceso correspondiente al turno actual
        size_t indice_proceso = procesos_ordenados[turno].indice_original;
        const proceso_t *proceso = &procesos[indice_proceso];

        // El proceso arranca cuando llega O cuando la CPU se desocupe (lo que sea mayor).
        int64_t tiempo_inicio =
            proceso->llegada > reloj_cpu ? proceso->llegada : reloj_cpu;

        // 1. ¿Llegó después del reloj? Entonces la CPU estuvo desocupada (CPU ociosa).
        if (proceso->llegada > reloj_cpu) {
            if (total_segmentos == SIZE_MAX) {
                return PLANIFICADOR_DESBORDAMIENTO_TIEMPO; // Freno de seguridad.
            }
            total_segmentos++; // +1 bloque por la CPU ociosa.
        }

        // 2. Control de seguridad contra números gigantes en la ráfaga.
        if (total_segmentos == SIZE_MAX
            || proceso->rafaga > INT64_MAX - tiempo_inicio) {
            return PLANIFICADOR_DESBORDAMIENTO_TIEMPO; // Freno de seguridad.
        }

        total_segmentos++; // +1 bloque por el proceso que se va a ejecutar.

        // Avanzamos el reloj de la CPU sumando lo que duró la ráfaga de este proceso.
        reloj_cpu = tiempo_inicio + proceso->rafaga;
    }

    // Le devolvemos a quien llamó la función el total final de bloques contados.
    *cantidad_segmentos_necesaria = total_segmentos;
    return PLANIFICADOR_OK;
}

/* ============================================================
 * ALGORITMO FCFS — INTERFAZ PUBLICA DEL NUCLEO
 * ============================================================ */
planificador_estado_t planificador_fcfs(
    const proceso_t *procesos,
    size_t cantidad,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas)
{
    proceso_ordenado_fcfs_t *procesos_ordenados;
    size_t cantidad_segmentos_necesaria;
    size_t indice_siguiente_segmento = 0;
    int64_t reloj_cpu = 0;
    planificador_estado_t estado;

    /* VALIDACIONES: revisar argumentos antes de acceder a los buffers. */
    if (cantidad_segmentos == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    *cantidad_segmentos = 0;

    if (procesos == NULL || cantidad == 0 || segmentos == NULL || metricas == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    if (cantidad > SIZE_MAX / sizeof(*procesos_ordenados)) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    /* Los tiempos deben ser validos antes de ordenar o calcular resultados. */
    for (size_t indice_proceso = 0; indice_proceso < cantidad; indice_proceso++) {
        if (procesos[indice_proceso].llegada < 0
            || procesos[indice_proceso].rafaga <= 0) {
            return PLANIFICADOR_ARGUMENTO_INVALIDO;
        }
    }

    /* Preparar una copia ordenable que conserva la posicion de cada proceso. */
    procesos_ordenados = malloc(cantidad * sizeof(*procesos_ordenados));
    if (procesos_ordenados == NULL) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    for (size_t indice_proceso = 0;
         indice_proceso < cantidad;
         indice_proceso++) {
        procesos_ordenados[indice_proceso].indice_original = indice_proceso;
        procesos_ordenados[indice_proceso].llegada =
            procesos[indice_proceso].llegada;
        procesos_ordenados[indice_proceso].prioridad =
            procesos[indice_proceso].prioridad;
    }
    qsort(
        procesos_ordenados,
        cantidad,
        sizeof(*procesos_ordenados),
        comparar_procesos_fcfs);

    /* Contar antes de escribir evita resultados parciales si faltan buffers. */
    estado = contar_segmentos_necesarios_fcfs(
        procesos,
        procesos_ordenados,
        cantidad,
        &cantidad_segmentos_necesaria);
    if (estado != PLANIFICADOR_OK) {
        free(procesos_ordenados);
        return estado;
    }
    *cantidad_segmentos = cantidad_segmentos_necesaria;
    if (capacidad_segmentos < cantidad_segmentos_necesaria
        || capacidad_metricas < cantidad) {
        free(procesos_ordenados);
        return PLANIFICADOR_CAPACIDAD_INSUFICIENTE;
    }

    /* ========================================================
     * CALCULO DE TIEMPOS Y SEGMENTOS DE CPU OCIOSA
     * ======================================================== */
    for (size_t turno = 0; turno < cantidad; turno++) {
        size_t indice_proceso = procesos_ordenados[turno].indice_original;
        const proceso_t *proceso = &procesos[indice_proceso];
        int64_t tiempo_inicio =
            proceso->llegada > reloj_cpu ? proceso->llegada : reloj_cpu;
        int64_t tiempo_fin = tiempo_inicio + proceso->rafaga;

        if (proceso->llegada > reloj_cpu) {
            segmentos[indice_siguiente_segmento++] = (segmento_t) {
                .proceso_id = 0,
                .inicio = reloj_cpu,
                .fin = proceso->llegada,
                .es_ocioso = 1
            };
        }

        segmentos[indice_siguiente_segmento++] = (segmento_t) {
            .proceso_id = proceso->id,
            .inicio = tiempo_inicio,
            .fin = tiempo_fin,
            .es_ocioso = 0
        };

        /* ====================================================
         * CALCULO DE METRICAS
         * ==================================================== */
        /* Las metricas quedan en el orden original de entrada. */
        metricas[indice_proceso] = (metricas_proceso_t) {
            .proceso_id = proceso->id,
            .espera = tiempo_inicio - proceso->llegada,
            .retorno = tiempo_fin - proceso->llegada,
            .respuesta = tiempo_inicio - proceso->llegada
        };
        reloj_cpu = tiempo_fin;
    }

    free(procesos_ordenados);
    return PLANIFICADOR_OK;
}

/* ============================================================
 * INTERFACES PUBLICAS Y ADAPTADORES
 * ============================================================
 * La interfaz v1 mantiene su firma actual y delega el calculo al nucleo.
 */
int32_t planificador_fcfs_v1(
    const proceso_t *procesos,
    int32_t cantidad_procesos,
    segmento_t *segmentos,
    int32_t capacidad_segmentos,
    int32_t *cantidad_segmentos_requerida,
    metricas_proceso_t *metricas,
    int32_t capacidad_metricas,
    int32_t *cantidad_metricas_requerida)
{
    segmento_t segmento_sonda;
    metricas_proceso_t metrica_sonda;
    size_t cantidad_segmentos_size = 0;
    planificador_estado_t estado;

    if (cantidad_segmentos_requerida != NULL) {
        *cantidad_segmentos_requerida = 0;
    }
    if (cantidad_metricas_requerida != NULL) {
        *cantidad_metricas_requerida = 0;
    }
    if (cantidad_segmentos_requerida == NULL
        || cantidad_metricas_requerida == NULL
        || procesos == NULL
        || segmentos == NULL
        || metricas == NULL
        || cantidad_procesos <= 0
        || capacidad_segmentos < 0
        || capacidad_metricas < 0) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
#if SIZE_MAX < INT32_MAX
    if ((uintmax_t)(uint32_t)cantidad_procesos > (uintmax_t)SIZE_MAX
        || (uintmax_t)(uint32_t)capacidad_segmentos > (uintmax_t)SIZE_MAX
        || (uintmax_t)(uint32_t)capacidad_metricas > (uintmax_t)SIZE_MAX) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
#endif

    estado = planificador_fcfs(
        procesos,
        (size_t)cantidad_procesos,
        &segmento_sonda,
        0,
        &cantidad_segmentos_size,
        &metrica_sonda,
        0);
    if (estado != PLANIFICADOR_CAPACIDAD_INSUFICIENTE) {
        return (int32_t)estado;
    }

    *cantidad_metricas_requerida = cantidad_procesos;
    if ((uintmax_t)cantidad_segmentos_size > (uintmax_t)INT32_MAX) {
        return PLANIFICADOR_CANTIDAD_NO_REPRESENTABLE;
    }
    *cantidad_segmentos_requerida = (int32_t)cantidad_segmentos_size;

    if ((size_t)capacidad_segmentos < cantidad_segmentos_size
        || capacidad_metricas < cantidad_procesos) {
        return PLANIFICADOR_CAPACIDAD_INSUFICIENTE;
    }

    estado = planificador_fcfs(
        procesos,
        (size_t)cantidad_procesos,
        segmentos,
        (size_t)capacidad_segmentos,
        &cantidad_segmentos_size,
        metricas,
        (size_t)capacidad_metricas);
    return (int32_t)estado;
}

/* ============================================================
 * FUTUROS ALGORITMOS
 * ============================================================
 * SJF no expropiativo.
 * SRTF expropiativo.
 * Round Robin con cuanto configurable.
 * Estos algoritmos no se implementan en esta etapa.
 */
