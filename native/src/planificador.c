#include "planificador.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

/* ============================================================
 * UTILIDADES COMUNES DEL NUCLEO
 * ============================================================ */
static planificador_estado_t sumar_tiempo_seguro(
    int64_t inicio,
    int64_t duracion,
    int64_t *fin)
{
    if (fin == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    if ((duracion > 0 && inicio > INT64_MAX - duracion)
        || (duracion < 0 && inicio < INT64_MIN - duracion)) {
        return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
    }

    *fin = inicio + duracion;
    return PLANIFICADOR_OK;
}

static metricas_proceso_t calcular_metricas_proceso(
    const proceso_t *proceso,
    int64_t inicio,
    int64_t fin)
{
    return (metricas_proceso_t) {
        .proceso_id = proceso->id,
        .espera = inicio - proceso->llegada,
        .retorno = fin - proceso->llegada,
        .respuesta = inicio - proceso->llegada
    };
}

/* ============================================================
 * ESTRUCTURAS AUXILIARES ESPECIFICAS DE FCFS
 * ============================================================ */
typedef struct {
    size_t indice_original;
    int64_t llegada;
    int32_t prioridad;
} proceso_ordenado_fcfs_t;

/* ============================================================
 * ORDENAMIENTO FCFS Y SUS DESEMPATES
 * ============================================================
 * FCFS ordena primero por llegada. La prioridad solo desempata
 * llegadas iguales; el indice original resuelve el ultimo empate.
 */
static int comparar_procesos_fcfs(const void *izquierda, const void *derecha)
{
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
 * PREPARACION DE LA PLANIFICACION FCFS
 * ============================================================ */
static planificador_estado_t contar_segmentos_necesarios_fcfs(
    const proceso_t *procesos,
    const proceso_ordenado_fcfs_t *procesos_ordenados,
    size_t cantidad_procesos,
    size_t *cantidad_segmentos_necesaria)
{
    int64_t reloj_cpu = 0;
    size_t total_segmentos = 0;

    for (size_t turno = 0; turno < cantidad_procesos; turno++) {
        size_t indice_proceso = procesos_ordenados[turno].indice_original;
        const proceso_t *proceso = &procesos[indice_proceso];
        int64_t tiempo_inicio =
            proceso->llegada > reloj_cpu ? proceso->llegada : reloj_cpu;
        int64_t tiempo_fin;

        if (proceso->llegada > reloj_cpu) {
            if (total_segmentos == SIZE_MAX) {
                return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
            }
            total_segmentos++;
        }

        if (total_segmentos == SIZE_MAX) {
            return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
        }
        total_segmentos++;

        planificador_estado_t estado = sumar_tiempo_seguro(
            tiempo_inicio,
            proceso->rafaga,
            &tiempo_fin);
        if (estado != PLANIFICADOR_OK) {
            return estado;
        }
        reloj_cpu = tiempo_fin;
    }

    *cantidad_segmentos_necesaria = total_segmentos;
    return PLANIFICADOR_OK;
}

/* ============================================================
 * ALGORITMO FCFS/FIFO
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

    /* Validar argumentos antes de acceder a los buffers. */
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

    /* Los tiempos deben ser válidos antes de ordenar o calcular resultados. */
    for (size_t indice_proceso = 0; indice_proceso < cantidad; indice_proceso++) {
        if (procesos[indice_proceso].llegada < 0
            || procesos[indice_proceso].rafaga <= 0) {
            return PLANIFICADOR_ARGUMENTO_INVALIDO;
        }
    }

    /* Preparar una copia ordenable que conserva la posición de cada proceso. */
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
     * GENERACION DE SEGMENTOS Y METRICAS
     * ======================================================== */
    for (size_t turno = 0; turno < cantidad; turno++) {
        size_t indice_proceso = procesos_ordenados[turno].indice_original;
        const proceso_t *proceso = &procesos[indice_proceso];
        int64_t tiempo_inicio =
            proceso->llegada > reloj_cpu ? proceso->llegada : reloj_cpu;
        int64_t tiempo_fin;

        estado = sumar_tiempo_seguro(
            tiempo_inicio,
            proceso->rafaga,
            &tiempo_fin);
        if (estado != PLANIFICADOR_OK) {
            free(procesos_ordenados);
            return estado;
        }

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

        /* Las métricas se guardan en el orden original de entrada. */
        metricas[indice_proceso] = calcular_metricas_proceso(
            proceso,
            tiempo_inicio,
            tiempo_fin);
        reloj_cpu = tiempo_fin;
    }

    free(procesos_ordenados);
    return PLANIFICADOR_OK;
}

/* ============================================================
 * ALGORITMO SJF NO EXPROPIATIVO
 * ============================================================ */
typedef struct {
    size_t indice_original;
    int64_t llegada;
    int64_t rafaga;
    int32_t prioridad;
    int64_t inicio;
    int64_t fin;
} proceso_sjf_t;

/*
 * Desempate provisional de SJF: llegada, prioridad y orden original.
 * La regla debe confirmarse con el profesor antes de considerarse definitiva.
 */
static int es_preferible_sjf(
    const proceso_sjf_t *candidato,
    const proceso_sjf_t *actual)
{
    if (candidato->rafaga != actual->rafaga) {
        return candidato->rafaga < actual->rafaga;
    }
    if (candidato->llegada != actual->llegada) {
        return candidato->llegada < actual->llegada;
    }
    if (candidato->prioridad != actual->prioridad) {
        return candidato->prioridad < actual->prioridad;
    }
    return candidato->indice_original < actual->indice_original;
}

planificador_estado_t planificador_sjf(
    const proceso_t *procesos,
    size_t cantidad,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas)
{
    proceso_sjf_t *planificacion;
    size_t total_segmentos = 0;
    int64_t reloj_cpu = 0;
    planificador_estado_t estado = PLANIFICADOR_OK;

    if (cantidad_segmentos == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    *cantidad_segmentos = 0;

    if (procesos == NULL || cantidad == 0 || segmentos == NULL || metricas == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    if (cantidad > SIZE_MAX / sizeof(*planificacion)) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    for (size_t i = 0; i < cantidad; i++) {
        if (procesos[i].llegada < 0 || procesos[i].rafaga <= 0) {
            return PLANIFICADOR_ARGUMENTO_INVALIDO;
        }
    }

    planificacion = malloc(cantidad * sizeof(*planificacion));
    if (planificacion == NULL) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    for (size_t i = 0; i < cantidad; i++) {
        planificacion[i] = (proceso_sjf_t) {
            .indice_original = i,
            .llegada = procesos[i].llegada,
            .rafaga = procesos[i].rafaga,
            .prioridad = procesos[i].prioridad,
            .inicio = 0,
            .fin = 0
        };
    }

    /*
     * Preparar todo el orden de ejecucion antes de escribir en los buffers
     * del llamador. Asi se pueden informar errores sin dejar resultados
     * parciales.
     */
    size_t turno = 0;
    while (turno < cantidad) {
        size_t indice_elegido = cantidad;

        for (size_t i = turno; i < cantidad; i++) {
            if (planificacion[i].llegada <= reloj_cpu
                && (indice_elegido == cantidad
                    || es_preferible_sjf(
                        &planificacion[i],
                        &planificacion[indice_elegido]))) {
                indice_elegido = i;
            }
        }

        if (indice_elegido == cantidad) {
            size_t indice_proxima_llegada = turno;
            for (size_t i = turno + 1; i < cantidad; i++) {
                if (planificacion[i].llegada
                    < planificacion[indice_proxima_llegada].llegada) {
                    indice_proxima_llegada = i;
                }
            }

            if (total_segmentos == SIZE_MAX) {
                estado = PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
                goto liberar_planificacion;
            }
            total_segmentos++;
            reloj_cpu = planificacion[indice_proxima_llegada].llegada;
            continue;
        }

        if (indice_elegido != turno) {
            proceso_sjf_t temporal = planificacion[turno];
            planificacion[turno] = planificacion[indice_elegido];
            planificacion[indice_elegido] = temporal;
        }

        if (total_segmentos == SIZE_MAX) {
            estado = PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
            goto liberar_planificacion;
        }
        total_segmentos++;

        planificacion[turno].inicio = reloj_cpu;
        estado = sumar_tiempo_seguro(
            reloj_cpu,
            planificacion[turno].rafaga,
            &planificacion[turno].fin);
        if (estado != PLANIFICADOR_OK) {
            goto liberar_planificacion;
        }
        reloj_cpu = planificacion[turno].fin;
        turno++;
    }

    *cantidad_segmentos = total_segmentos;
    if (capacidad_segmentos < total_segmentos
        || capacidad_metricas < cantidad) {
        estado = PLANIFICADOR_CAPACIDAD_INSUFICIENTE;
        goto liberar_planificacion;
    }

    /* Escribir segmentos de CPU y métricas en orden de entrada. */
    size_t indice_siguiente_segmento = 0;
    reloj_cpu = 0;
    for (size_t turno = 0; turno < cantidad; turno++) {
        const proceso_sjf_t *proceso_planificado = &planificacion[turno];
        const proceso_t *proceso =
            &procesos[proceso_planificado->indice_original];

        if (proceso_planificado->inicio > reloj_cpu) {
            segmentos[indice_siguiente_segmento++] = (segmento_t) {
                .proceso_id = 0,
                .inicio = reloj_cpu,
                .fin = proceso_planificado->inicio,
                .es_ocioso = 1
            };
        }

        segmentos[indice_siguiente_segmento++] = (segmento_t) {
            .proceso_id = proceso->id,
            .inicio = proceso_planificado->inicio,
            .fin = proceso_planificado->fin,
            .es_ocioso = 0
        };
        metricas[proceso_planificado->indice_original] =
            calcular_metricas_proceso(
                proceso,
                proceso_planificado->inicio,
                proceso_planificado->fin);
        reloj_cpu = proceso_planificado->fin;
    }

liberar_planificacion:
    free(planificacion);
    return estado;
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
 * SRTF expropiativo.
 * Round Robin con cuanto configurable.
 * Estos algoritmos no se implementan en esta etapa.
 */
