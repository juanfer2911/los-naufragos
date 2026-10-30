#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int32_t id;
    int64_t llegada;
    int64_t rafaga;
    int32_t prioridad;
} proceso_t;

typedef struct {
    int32_t proceso_id;
    int64_t inicio;
    int64_t fin;
    int32_t es_ocioso;
} segmento_t;

typedef struct {
    int32_t proceso_id;
    int64_t espera;
    int64_t retorno;
    int64_t respuesta;
} metricas_proceso_t;

typedef enum {
    PLANIFICADOR_OK = 0,
    PLANIFICADOR_ARGUMENTO_INVALIDO = 1,
    PLANIFICADOR_MEMORIA_INSUFICIENTE = 2,
    PLANIFICADOR_CAPACIDAD_INSUFICIENTE = 3,
    PLANIFICADOR_DESBORDAMIENTO_TIEMPO = 4,
    PLANIFICADOR_CANTIDAD_NO_REPRESENTABLE = 5
} planificador_estado_t;

/*
 * Codigos numericos de la interfaz v1:
 * 0 OK, 1 argumento invalido, 2 memoria insuficiente,
 * 3 capacidad insuficiente, 4 desbordamiento de tiempo,
 * 5 cantidad requerida de segmentos no representable en int32_t.
 */

/*
 * Ejecuta FCFS/FIFO sin expropiacion.
 * Ordena por llegada; si las llegadas empatan, usa menor prioridad numerica
 * y luego el orden original. Las metricas se escriben en orden de entrada.
 * Los segmentos ociosos usan es_ocioso = 1.
 *
 * Esta interfaz permite probar el nucleo nativo; la ABI para JNA sigue
 * pendiente de acuerdo.
 */
planificador_estado_t planificador_fcfs(
    const proceso_t *procesos,
    size_t cantidad,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas);

/*
 * Interfaz versionada con cantidades y capacidades int32_t.
 *
 * Ambos contadores de salida son obligatorios. Si alguno es NULL, devuelve
 * PLANIFICADOR_ARGUMENTO_INVALIDO, pone a cero cualquier contador no nulo
 * y no modifica los buffers de resultados.
 * Los buffers de procesos, segmentos y metricas deben ser no nulos incluso
 * cuando su capacidad sea cero.
 * Si el sondeo falla antes de calcular los tamanos, ambos contadores quedan
 * en cero. Ante capacidad insuficiente, informa ambos tamanos requeridos y
 * no escribe resultados parciales. Si la segunda llamada falla, conserva en
 * los contadores los requerimientos calculados por el sondeo.
 * Si la cantidad requerida de segmentos excede INT32_MAX, devuelve
 * PLANIFICADOR_CANTIDAD_NO_REPRESENTABLE, deja ese contador en cero y
 * conserva la cantidad requerida de metricas.
 */
int32_t planificador_fcfs_v1(
    const proceso_t *procesos,
    int32_t cantidad_procesos,
    segmento_t *segmentos,
    int32_t capacidad_segmentos,
    int32_t *cantidad_segmentos_requerida,
    metricas_proceso_t *metricas,
    int32_t capacidad_metricas,
    int32_t *cantidad_metricas_requerida);

#endif
