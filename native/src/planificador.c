#include "planificador.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    size_t indice_original;
    int64_t llegada;
    int32_t prioridad;
} proceso_ordenado_t;

static int comparar_procesos(const void *izquierda, const void *derecha)
{
    const proceso_ordenado_t *a = izquierda;
    const proceso_ordenado_t *b = derecha;

    if (a->llegada < b->llegada) {
        return -1;
    }
    if (a->llegada > b->llegada) {
        return 1;
    }
    if (a->prioridad < b->prioridad) {
        return -1;
    }
    if (a->prioridad > b->prioridad) {
        return 1;
    }
    if (a->indice_original < b->indice_original) {
        return -1;
    }
    if (a->indice_original > b->indice_original) {
        return 1;
    }
    return 0;
}

static planificador_estado_t contar_segmentos(
    const proceso_t *procesos,
    const proceso_ordenado_t *orden,
    size_t cantidad,
    size_t *resultado)
{
    int64_t reloj = 0;
    size_t total = 0;

    for (size_t i = 0; i < cantidad; i++) {
        const proceso_t *proceso = &procesos[orden[i].indice_original];
        int64_t inicio = proceso->llegada > reloj ? proceso->llegada : reloj;

        if (proceso->llegada > reloj) {
            if (total == SIZE_MAX) {
                return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
            }
            total++;
        }
        if (total == SIZE_MAX || proceso->rafaga > INT64_MAX - inicio) {
            return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
        }
        total++;
        reloj = inicio + proceso->rafaga;
    }

    *resultado = total;
    return PLANIFICADOR_OK;
}

planificador_estado_t planificador_fcfs(
    const proceso_t *procesos,
    size_t cantidad,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas)
{
    proceso_ordenado_t *orden;
    size_t necesarios;
    size_t siguiente_segmento = 0;
    int64_t reloj = 0;
    planificador_estado_t estado;

    if (cantidad_segmentos == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    *cantidad_segmentos = 0;

    if (procesos == NULL || cantidad == 0 || segmentos == NULL || metricas == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    if (cantidad > SIZE_MAX / sizeof(*orden)) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    for (size_t i = 0; i < cantidad; i++) {
        if (procesos[i].llegada < 0 || procesos[i].rafaga <= 0) {
            return PLANIFICADOR_ARGUMENTO_INVALIDO;
        }
    }

    orden = malloc(cantidad * sizeof(*orden));
    if (orden == NULL) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    for (size_t i = 0; i < cantidad; i++) {
        orden[i].indice_original = i;
        orden[i].llegada = procesos[i].llegada;
        orden[i].prioridad = procesos[i].prioridad;
    }
    qsort(orden, cantidad, sizeof(*orden), comparar_procesos);

    estado = contar_segmentos(procesos, orden, cantidad, &necesarios);
    if (estado != PLANIFICADOR_OK) {
        free(orden);
        return estado;
    }
    *cantidad_segmentos = necesarios;
    if (capacidad_segmentos < necesarios || capacidad_metricas < cantidad) {
        free(orden);
        return PLANIFICADOR_CAPACIDAD_INSUFICIENTE;
    }

    for (size_t i = 0; i < cantidad; i++) {
        size_t indice = orden[i].indice_original;
        const proceso_t *proceso = &procesos[indice];
        int64_t inicio = proceso->llegada > reloj ? proceso->llegada : reloj;
        int64_t fin = inicio + proceso->rafaga;

        if (proceso->llegada > reloj) {
            segmentos[siguiente_segmento++] = (segmento_t) {
                .proceso_id = 0,
                .inicio = reloj,
                .fin = proceso->llegada,
                .es_ocioso = 1
            };
        }

        segmentos[siguiente_segmento++] = (segmento_t) {
            .proceso_id = proceso->id,
            .inicio = inicio,
            .fin = fin,
            .es_ocioso = 0
        };
        metricas[indice] = (metricas_proceso_t) {
            .proceso_id = proceso->id,
            .espera = inicio - proceso->llegada,
            .retorno = fin - proceso->llegada,
            .respuesta = inicio - proceso->llegada
        };
        reloj = fin;
    }

    free(orden);
    return PLANIFICADOR_OK;
}
