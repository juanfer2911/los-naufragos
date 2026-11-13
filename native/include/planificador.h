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
 * API interna con size_t; usar planificador_ejecutar_v1 para JNA.
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
 * Ejecuta SJF no expropiativo.
 * Elige la menor rafaga entre los procesos que ya llegaron y ejecuta el
 * proceso seleccionado hasta finalizar. En empates de rafaga se propone
 * usar llegada menor, prioridad numerica menor y orden original, en ese
 * orden; esta politica esta pendiente de confirmacion con el profesor.
 * Las metricas se escriben en orden de entrada y los periodos ociosos usan
 * es_ocioso = 1. Ante capacidad insuficiente informa el total requerido de
 * segmentos sin modificar los buffers de resultados.
 */
planificador_estado_t planificador_sjf(
    const proceso_t *procesos,
    size_t cantidad,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas);

/*
 * Ejecuta SRTF de forma expropiativa y basada en eventos de llegada o
 * finalizacion, sin avanzar unidad por unidad. El menor tiempo restante es
 * el criterio principal. Si el proceso actual empata con otro en tiempo
 * restante, conserva la CPU; sin proceso actual, se propone desempatar por
 * llegada menor, prioridad numerica menor y orden original. Estos criterios
 * son provisionales y estan pendientes de confirmacion con el profesor.
 * Las metricas se escriben en orden de entrada. Ante capacidad insuficiente
 * informa el total requerido de segmentos sin modificar los buffers.
 */
planificador_estado_t planificador_srtf(
    const proceso_t *procesos,
    size_t cantidad,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas);

/*
 * Ejecuta Round Robin con quantum positivo, expresado en las mismas
 * unidades que llegada y rafaga. Las llegadas se incorporan a la cola por
 * llegada ascendente y, en empate, orden original. Al vencer el quantum,
 * primero se agregan las llegadas con tiempo <= reloj y luego se reinserta
 * el proceso si sigue pendiente. La prioridad no altera el orden FIFO.
 * Las metricas se escriben en orden de entrada. Ante capacidad insuficiente
 * informa el total requerido de segmentos sin modificar los buffers.
 */
planificador_estado_t planificador_rr(
    const proceso_t *procesos,
    size_t cantidad,
    int64_t quantum,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas);

/* Cadenas ASCII terminadas en cero. Disposicion estable para JNA. */
typedef struct {
    char id[16];
    char nombre[64];
    int32_t requiere_quantum;
} algoritmo_t;

/* Memoria estatica de solo lectura, valida durante la vida de la biblioteca.
 * No liberar. Con contador NULL devuelve NULL.
 */
const algoritmo_t *listar_algoritmos(int32_t *cantidad);

/* Identificadores del catalogo; quantum solo se usa en rr.
 * Buffers NULL permitidos con capacidad cero. Sondeo devuelve estado 3
 * y ambos tamanos; sin escrituras parciales por falta de capacidad.
 * Contadores obligatorios y distintos; buffers sin solapamiento.
 * Ante error previo al sondeo quedan en cero; estado 5 conserva metricas
 * y deja segmentos en cero. Un error posterior conserva los tamanos.
 */
int32_t planificador_ejecutar_v1(
    const char *algoritmo_id,
    const proceso_t *procesos,
    int32_t cantidad_procesos,
    int64_t quantum,
    segmento_t *segmentos,
    int32_t capacidad_segmentos,
    int32_t *cantidad_segmentos_requerida,
    metricas_proceso_t *metricas,
    int32_t capacidad_metricas,
    int32_t *cantidad_metricas_requerida);

#endif
