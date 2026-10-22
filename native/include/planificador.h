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
    PLANIFICADOR_DESBORDAMIENTO_TIEMPO = 4
} planificador_estado_t;

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

#endif
