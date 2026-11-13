#include "planificador.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

static metricas_proceso_t calcular_metricas(
    const proceso_t *proceso,
    int64_t primer_inicio,
    int64_t finalizacion)
{
    int64_t retorno = finalizacion - proceso->llegada;
    return (metricas_proceso_t) {
        .proceso_id = proceso->id,
        .espera = retorno - proceso->rafaga,
        .retorno = retorno,
        .respuesta = primer_inicio - proceso->llegada
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
        metricas[indice_proceso] = calcular_metricas(
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

    /* Calcula primero el plan para no dejar resultados parciales. */
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
            calcular_metricas(
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
 * ALGORITMO SRTF EXPROPIATIVO
 * ============================================================ */
typedef struct {
    size_t indice_original;
    int64_t llegada;
    int64_t restante;
    int32_t prioridad;
    int64_t primer_inicio;
    int64_t finalizacion;
} proceso_srtf_t;

/*
 * Cuando no se conserva el proceso actual, los empates se resuelven por
 * llegada, prioridad y orden original. La politica es provisional.
 */
static int es_preferible_srtf(
    const proceso_srtf_t *candidato,
    const proceso_srtf_t *actual)
{
    if (candidato->restante != actual->restante) {
        return candidato->restante < actual->restante;
    }
    if (candidato->llegada != actual->llegada) {
        return candidato->llegada < actual->llegada;
    }
    if (candidato->prioridad != actual->prioridad) {
        return candidato->prioridad < actual->prioridad;
    }
    return candidato->indice_original < actual->indice_original;
}

static size_t seleccionar_proceso_srtf(
    const proceso_srtf_t *planificacion,
    size_t cantidad,
    int64_t reloj_cpu,
    size_t proceso_actual)
{
    size_t indice_elegido = cantidad;

    for (size_t i = 0; i < cantidad; i++) {
        if (planificacion[i].restante > 0
            && planificacion[i].llegada <= reloj_cpu
            && (indice_elegido == cantidad
                || es_preferible_srtf(
                    &planificacion[i],
                    &planificacion[indice_elegido]))) {
            indice_elegido = i;
        }
    }

    if (proceso_actual < cantidad
        && planificacion[proceso_actual].restante > 0
        && planificacion[proceso_actual].llegada <= reloj_cpu
        && indice_elegido < cantidad
        && planificacion[proceso_actual].restante
           == planificacion[indice_elegido].restante) {
        return proceso_actual;
    }
    return indice_elegido;
}

/* Simulación por eventos; nunca escribe directamente en buffers públicos. */
static planificador_estado_t simular_srtf(
    const proceso_t *procesos,
    proceso_srtf_t *planificacion,
    size_t cantidad,
    segmento_t *segmentos_temporales,
    metricas_proceso_t *metricas_temporales,
    size_t *cantidad_segmentos)
{
    size_t terminados = 0;
    size_t total_segmentos = 0;
    size_t ultimo_indice_proceso = cantidad;
    size_t proceso_actual = cantidad;
    int ultimo_segmento_ocioso = 0;
    int64_t reloj_cpu = 0;

    for (size_t i = 0; i < cantidad; i++) {
        planificacion[i] = (proceso_srtf_t) {
            .indice_original = i,
            .llegada = procesos[i].llegada,
            .restante = procesos[i].rafaga,
            .prioridad = procesos[i].prioridad,
            .primer_inicio = -1,
            .finalizacion = -1
        };
    }

    while (terminados < cantidad) {
        size_t elegido = seleccionar_proceso_srtf(
            planificacion,
            cantidad,
            reloj_cpu,
            proceso_actual);

        if (elegido == cantidad) {
            size_t indice_proxima_llegada = cantidad;
            for (size_t i = 0; i < cantidad; i++) {
                if (planificacion[i].restante > 0
                    && planificacion[i].llegada > reloj_cpu
                    && (indice_proxima_llegada == cantidad
                        || planificacion[i].llegada
                           < planificacion[indice_proxima_llegada].llegada)) {
                    indice_proxima_llegada = i;
                }
            }
            if (indice_proxima_llegada == cantidad) {
                return PLANIFICADOR_ARGUMENTO_INVALIDO;
            }

            int64_t fin_ocio = planificacion[indice_proxima_llegada].llegada;
            if (total_segmentos == SIZE_MAX) {
                return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
            }
            if (segmentos_temporales != NULL) {
                segmentos_temporales[total_segmentos] = (segmento_t) {
                    .proceso_id = 0,
                    .inicio = reloj_cpu,
                    .fin = fin_ocio,
                    .es_ocioso = 1
                };
            }
            total_segmentos++;
            ultimo_segmento_ocioso = 1;
            ultimo_indice_proceso = cantidad;
            reloj_cpu = fin_ocio;
            proceso_actual = cantidad;
            continue;
        }

        if (planificacion[elegido].primer_inicio < 0) {
            planificacion[elegido].primer_inicio = reloj_cpu;
        }

        size_t indice_proxima_llegada = cantidad;
        for (size_t i = 0; i < cantidad; i++) {
            if (planificacion[i].restante > 0
                && planificacion[i].llegada > reloj_cpu
                && (indice_proxima_llegada == cantidad
                    || planificacion[i].llegada
                       < planificacion[indice_proxima_llegada].llegada)) {
                indice_proxima_llegada = i;
            }
        }

        int64_t fin_proceso;
        planificador_estado_t estado = sumar_tiempo_seguro(
            reloj_cpu,
            planificacion[elegido].restante,
            &fin_proceso);
        if (estado != PLANIFICADOR_OK
            && indice_proxima_llegada == cantidad) {
            return estado;
        }

        int64_t fin_intervalo;
        if (estado != PLANIFICADOR_OK
            || (indice_proxima_llegada < cantidad
                && planificacion[indice_proxima_llegada].llegada < fin_proceso)) {
            fin_intervalo = planificacion[indice_proxima_llegada].llegada;
        } else {
            fin_intervalo = fin_proceso;
        }

        int64_t duracion = fin_intervalo - reloj_cpu;
        if (duracion <= 0 || duracion > planificacion[elegido].restante) {
            return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
        }

        if (!ultimo_segmento_ocioso
            && ultimo_indice_proceso == elegido
            && total_segmentos > 0) {
            if (segmentos_temporales != NULL) {
                segmentos_temporales[total_segmentos - 1].fin = fin_intervalo;
            }
        } else {
            if (total_segmentos == SIZE_MAX) {
                return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
            }
            if (segmentos_temporales != NULL) {
                segmentos_temporales[total_segmentos] = (segmento_t) {
                    .proceso_id = procesos[planificacion[elegido].indice_original].id,
                    .inicio = reloj_cpu,
                    .fin = fin_intervalo,
                    .es_ocioso = 0
                };
            }
            total_segmentos++;
        }

        ultimo_segmento_ocioso = 0;
        ultimo_indice_proceso = elegido;
        planificacion[elegido].restante -= duracion;
        reloj_cpu = fin_intervalo;
        if (planificacion[elegido].restante == 0) {
            planificacion[elegido].finalizacion = reloj_cpu;
            terminados++;
            proceso_actual = cantidad;
        } else {
            proceso_actual = elegido;
        }
    }

    if (metricas_temporales != NULL) {
        for (size_t i = 0; i < cantidad; i++) {
            metricas_temporales[planificacion[i].indice_original] =
                calcular_metricas(
                    &procesos[planificacion[i].indice_original],
                    planificacion[i].primer_inicio,
                    planificacion[i].finalizacion);
        }
    }
    *cantidad_segmentos = total_segmentos;
    return PLANIFICADOR_OK;
}

planificador_estado_t planificador_srtf(
    const proceso_t *procesos,
    size_t cantidad,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas)
{
    proceso_srtf_t *planificacion;
    segmento_t *segmentos_temporales = NULL;
    metricas_proceso_t *metricas_temporales = NULL;
    size_t cantidad_segmentos_calculada = 0;
    planificador_estado_t estado;

    if (cantidad_segmentos == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    *cantidad_segmentos = 0;
    if (procesos == NULL || cantidad == 0 || segmentos == NULL || metricas == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    if (cantidad > SIZE_MAX / sizeof(*planificacion)
        || cantidad > SIZE_MAX / sizeof(*metricas_temporales)) {
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

    estado = simular_srtf(
        procesos,
        planificacion,
        cantidad,
        NULL,
        NULL,
        &cantidad_segmentos_calculada);
    if (estado != PLANIFICADOR_OK) {
        free(planificacion);
        return estado;
    }

    if (capacidad_segmentos < cantidad_segmentos_calculada
        || capacidad_metricas < cantidad) {
        *cantidad_segmentos = cantidad_segmentos_calculada;
        free(planificacion);
        return PLANIFICADOR_CAPACIDAD_INSUFICIENTE;
    }
    if (cantidad_segmentos_calculada > SIZE_MAX / sizeof(*segmentos_temporales)) {
        free(planificacion);
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    segmentos_temporales =
        malloc(cantidad_segmentos_calculada * sizeof(*segmentos_temporales));
    metricas_temporales = malloc(cantidad * sizeof(*metricas_temporales));
    if (segmentos_temporales == NULL || metricas_temporales == NULL) {
        free(segmentos_temporales);
        free(metricas_temporales);
        free(planificacion);
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    estado = simular_srtf(
        procesos,
        planificacion,
        cantidad,
        segmentos_temporales,
        metricas_temporales,
        &cantidad_segmentos_calculada);
    if (estado == PLANIFICADOR_OK) {
        for (size_t i = 0; i < cantidad_segmentos_calculada; i++) {
            segmentos[i] = segmentos_temporales[i];
        }
        for (size_t i = 0; i < cantidad; i++) {
            metricas[i] = metricas_temporales[i];
        }
        *cantidad_segmentos = cantidad_segmentos_calculada;
    }

    free(segmentos_temporales);
    free(metricas_temporales);
    free(planificacion);
    return estado;
}

/* ============================================================
 * ALGORITMO ROUND ROBIN CON QUANTUM CONFIGURABLE
 * ============================================================ */
typedef struct {
    size_t indice_original;
    int64_t restante;
    int64_t primer_inicio;
    int64_t finalizacion;
} proceso_rr_t;

typedef struct {
    int64_t llegada;
    size_t indice_original;
} llegada_rr_t;

static int comparar_llegadas_rr(const void *izquierda, const void *derecha)
{
    const llegada_rr_t *llegada_izquierda = izquierda;
    const llegada_rr_t *llegada_derecha = derecha;

    if (llegada_izquierda->llegada < llegada_derecha->llegada) {
        return -1;
    }
    if (llegada_izquierda->llegada > llegada_derecha->llegada) {
        return 1;
    }
    if (llegada_izquierda->indice_original
        < llegada_derecha->indice_original) {
        return -1;
    }
    if (llegada_izquierda->indice_original
        > llegada_derecha->indice_original) {
        return 1;
    }
    return 0;
}

static planificador_estado_t encolar_rr(
    size_t *cola,
    size_t capacidad,
    size_t *final,
    size_t *cantidad_en_cola,
    size_t indice_proceso)
{
    if (*cantidad_en_cola >= capacidad) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }
    cola[*final] = indice_proceso;
    *final = (*final + 1) % capacidad;
    (*cantidad_en_cola)++;
    return PLANIFICADOR_OK;
}

static planificador_estado_t agregar_llegadas_rr(
    const llegada_rr_t *orden_llegadas,
    size_t cantidad,
    int64_t reloj_cpu,
    size_t *siguiente_llegada,
    size_t *cola,
    size_t capacidad_cola,
    size_t *final,
    size_t *cantidad_en_cola)
{
    while (*siguiente_llegada < cantidad
        && orden_llegadas[*siguiente_llegada].llegada <= reloj_cpu) {
        size_t indice = orden_llegadas[*siguiente_llegada].indice_original;
        planificador_estado_t estado = encolar_rr(
            cola,
            capacidad_cola,
            final,
            cantidad_en_cola,
            indice);
        if (estado != PLANIFICADOR_OK) {
            return estado;
        }
        (*siguiente_llegada)++;
    }
    return PLANIFICADOR_OK;
}

/*
 * Simula por turnos y admite una primera pasada sin buffers para contar
 * segmentos antes de preparar los resultados públicos.
 */
static planificador_estado_t simular_rr(
    const proceso_t *procesos,
    size_t cantidad,
    int64_t quantum,
    proceso_rr_t *estado_procesos,
    const llegada_rr_t *orden_llegadas,
    size_t *cola,
    segmento_t *segmentos_temporales,
    metricas_proceso_t *metricas_temporales,
    size_t *cantidad_segmentos)
{
    size_t frente = 0;
    size_t final = 0;
    size_t cantidad_en_cola = 0;
    size_t siguiente_llegada = 0;
    size_t terminados = 0;
    size_t total_segmentos = 0;
    size_t ultimo_indice_segmento = cantidad;
    int ultimo_segmento_ocioso = 0;
    int64_t fin_ultimo_segmento = 0;
    int64_t reloj_cpu = 0;

    for (size_t i = 0; i < cantidad; i++) {
        estado_procesos[i] = (proceso_rr_t) {
            .indice_original = i,
            .restante = procesos[i].rafaga,
            .primer_inicio = -1,
            .finalizacion = -1
        };
    }

    while (terminados < cantidad) {
        planificador_estado_t estado = agregar_llegadas_rr(
            orden_llegadas,
            cantidad,
            reloj_cpu,
            &siguiente_llegada,
            cola,
            cantidad,
            &final,
            &cantidad_en_cola);
        if (estado != PLANIFICADOR_OK) {
            return estado;
        }

        if (cantidad_en_cola == 0) {
            if (siguiente_llegada >= cantidad) {
                return PLANIFICADOR_ARGUMENTO_INVALIDO;
            }

            int64_t proxima_llegada =
                orden_llegadas[siguiente_llegada].llegada;
            if (proxima_llegada <= reloj_cpu) {
                return PLANIFICADOR_ARGUMENTO_INVALIDO;
            }
            if (total_segmentos == SIZE_MAX) {
                return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
            }
            if (segmentos_temporales != NULL) {
                segmentos_temporales[total_segmentos] = (segmento_t) {
                    .proceso_id = 0,
                    .inicio = reloj_cpu,
                    .fin = proxima_llegada,
                    .es_ocioso = 1
                };
            }
            total_segmentos++;
            ultimo_segmento_ocioso = 1;
            ultimo_indice_segmento = cantidad;
            fin_ultimo_segmento = proxima_llegada;
            reloj_cpu = proxima_llegada;
            continue;
        }

        size_t indice_proceso = cola[frente];
        frente = (frente + 1) % cantidad;
        cantidad_en_cola--;
        proceso_rr_t *proceso_actual = &estado_procesos[indice_proceso];

        if (proceso_actual->primer_inicio < 0) {
            proceso_actual->primer_inicio = reloj_cpu;
        }

        int64_t duracion = proceso_actual->restante < quantum
                               ? proceso_actual->restante
                               : quantum;
        /* Sin competencia antes de terminar, no hace falta simular quanta. */
        if (cantidad_en_cola == 0
            && (siguiente_llegada == cantidad
                || proceso_actual->restante
                   <= orden_llegadas[siguiente_llegada].llegada - reloj_cpu)) {
            duracion = proceso_actual->restante;
        }
        int64_t fin_turno;
        estado = sumar_tiempo_seguro(reloj_cpu, duracion, &fin_turno);
        if (estado != PLANIFICADOR_OK) {
            return estado;
        }

        int fusionar_segmento =
            !ultimo_segmento_ocioso
            && ultimo_indice_segmento == indice_proceso
            && fin_ultimo_segmento == reloj_cpu;
        if (fusionar_segmento) {
            if (segmentos_temporales != NULL) {
                segmentos_temporales[total_segmentos - 1].fin = fin_turno;
            }
        } else {
            if (total_segmentos == SIZE_MAX) {
                return PLANIFICADOR_DESBORDAMIENTO_TIEMPO;
            }
            if (segmentos_temporales != NULL) {
                segmentos_temporales[total_segmentos] = (segmento_t) {
                    .proceso_id = procesos[indice_proceso].id,
                    .inicio = reloj_cpu,
                    .fin = fin_turno,
                    .es_ocioso = 0
                };
            }
            total_segmentos++;
        }
        ultimo_segmento_ocioso = 0;
        ultimo_indice_segmento = indice_proceso;
        fin_ultimo_segmento = fin_turno;

        proceso_actual->restante -= duracion;
        reloj_cpu = fin_turno;
        if (proceso_actual->restante == 0) {
            proceso_actual->finalizacion = reloj_cpu;
            terminados++;
        }

        /* Las llegadas al vencer el quantum entran antes del proceso actual. */
        estado = agregar_llegadas_rr(
            orden_llegadas,
            cantidad,
            reloj_cpu,
            &siguiente_llegada,
            cola,
            cantidad,
            &final,
            &cantidad_en_cola);
        if (estado != PLANIFICADOR_OK) {
            return estado;
        }

        if (proceso_actual->restante > 0) {
            estado = encolar_rr(
                cola,
                cantidad,
                &final,
                &cantidad_en_cola,
                indice_proceso);
            if (estado != PLANIFICADOR_OK) {
                return estado;
            }
        }
    }

    if (metricas_temporales != NULL) {
        for (size_t i = 0; i < cantidad; i++) {
            size_t indice = estado_procesos[i].indice_original;
            metricas_temporales[indice] = calcular_metricas(
                &procesos[indice],
                estado_procesos[i].primer_inicio,
                estado_procesos[i].finalizacion);
        }
    }
    *cantidad_segmentos = total_segmentos;
    return PLANIFICADOR_OK;
}

planificador_estado_t planificador_rr(
    const proceso_t *procesos,
    size_t cantidad,
    int64_t quantum,
    segmento_t *segmentos,
    size_t capacidad_segmentos,
    size_t *cantidad_segmentos,
    metricas_proceso_t *metricas,
    size_t capacidad_metricas)
{
    proceso_rr_t *estado_procesos;
    llegada_rr_t *orden_llegadas;
    size_t *cola;
    segmento_t *segmentos_temporales = NULL;
    metricas_proceso_t *metricas_temporales = NULL;
    size_t cantidad_segmentos_calculada = 0;
    planificador_estado_t estado;

    if (cantidad_segmentos == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    *cantidad_segmentos = 0;
    if (procesos == NULL
        || cantidad == 0
        || quantum <= 0
        || segmentos == NULL
        || metricas == NULL) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    if (cantidad > SIZE_MAX / sizeof(*estado_procesos)
        || cantidad > SIZE_MAX / sizeof(*orden_llegadas)
        || cantidad > SIZE_MAX / sizeof(*cola)
        || cantidad > SIZE_MAX / sizeof(*metricas_temporales)) {
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    estado_procesos = malloc(cantidad * sizeof(*estado_procesos));
    orden_llegadas = malloc(cantidad * sizeof(*orden_llegadas));
    cola = malloc(cantidad * sizeof(*cola));
    if (estado_procesos == NULL || orden_llegadas == NULL || cola == NULL) {
        free(estado_procesos);
        free(orden_llegadas);
        free(cola);
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    for (size_t i = 0; i < cantidad; i++) {
        if (procesos[i].llegada < 0 || procesos[i].rafaga <= 0) {
            free(estado_procesos);
            free(orden_llegadas);
            free(cola);
            return PLANIFICADOR_ARGUMENTO_INVALIDO;
        }
        orden_llegadas[i] = (llegada_rr_t) {
            .llegada = procesos[i].llegada,
            .indice_original = i
        };
    }
    qsort(
        orden_llegadas,
        cantidad,
        sizeof(*orden_llegadas),
        comparar_llegadas_rr);

    estado = simular_rr(
        procesos,
        cantidad,
        quantum,
        estado_procesos,
        orden_llegadas,
        cola,
        NULL,
        NULL,
        &cantidad_segmentos_calculada);
    if (estado != PLANIFICADOR_OK) {
        free(estado_procesos);
        free(orden_llegadas);
        free(cola);
        return estado;
    }

    if (capacidad_segmentos < cantidad_segmentos_calculada
        || capacidad_metricas < cantidad) {
        *cantidad_segmentos = cantidad_segmentos_calculada;
        free(estado_procesos);
        free(orden_llegadas);
        free(cola);
        return PLANIFICADOR_CAPACIDAD_INSUFICIENTE;
    }
    if (cantidad_segmentos_calculada > SIZE_MAX / sizeof(*segmentos_temporales)) {
        free(estado_procesos);
        free(orden_llegadas);
        free(cola);
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    segmentos_temporales =
        malloc(cantidad_segmentos_calculada * sizeof(*segmentos_temporales));
    metricas_temporales = malloc(cantidad * sizeof(*metricas_temporales));
    if (segmentos_temporales == NULL || metricas_temporales == NULL) {
        free(segmentos_temporales);
        free(metricas_temporales);
        free(estado_procesos);
        free(orden_llegadas);
        free(cola);
        return PLANIFICADOR_MEMORIA_INSUFICIENTE;
    }

    estado = simular_rr(
        procesos,
        cantidad,
        quantum,
        estado_procesos,
        orden_llegadas,
        cola,
        segmentos_temporales,
        metricas_temporales,
        &cantidad_segmentos_calculada);
    if (estado == PLANIFICADOR_OK) {
        for (size_t i = 0; i < cantidad_segmentos_calculada; i++) {
            segmentos[i] = segmentos_temporales[i];
        }
        for (size_t i = 0; i < cantidad; i++) {
            metricas[i] = metricas_temporales[i];
        }
        *cantidad_segmentos = cantidad_segmentos_calculada;
    }

    free(segmentos_temporales);
    free(metricas_temporales);
    free(estado_procesos);
    free(orden_llegadas);
    free(cola);
    return estado;
}

/* ============================================================
 * INTERFACES PUBLICAS Y ADAPTADORES
 * ============================================================
 * La interfaz comun v1 delega el calculo al nucleo.
 */
static const algoritmo_t catalogo[] = {
    {"fcfs", "FCFS/FIFO", 0},
    {"sjf", "SJF", 0},
    {"srtf", "SRTF", 0},
    {"rr", "Round Robin", 1}
};

const algoritmo_t *listar_algoritmos(int32_t *cantidad)
{
    if (cantidad == NULL) {
        return NULL;
    }
    *cantidad = (int32_t)(sizeof(catalogo) / sizeof(catalogo[0]));
    return catalogo;
}

/* Seleccion interna: las reglas permanecen en cada algoritmo. */
static planificador_estado_t ejecutar_seleccionado(
    int algoritmo, int64_t quantum, const proceso_t *procesos,
    size_t cantidad, segmento_t *segmentos, size_t capacidad_segmentos,
    size_t *cantidad_segmentos, metricas_proceso_t *metricas,
    size_t capacidad_metricas)
{
    if (algoritmo == 3) {
        return planificador_rr(procesos, cantidad, quantum, segmentos,
            capacidad_segmentos, cantidad_segmentos, metricas, capacidad_metricas);
    }
    typedef planificador_estado_t (*planificador_t)(const proceso_t *, size_t,
        segmento_t *, size_t, size_t *, metricas_proceso_t *, size_t);
    const planificador_t funciones[] = {
        planificador_fcfs, planificador_sjf, planificador_srtf
    };
    return funciones[algoritmo](procesos, cantidad, segmentos,
        capacidad_segmentos, cantidad_segmentos, metricas, capacidad_metricas);
}

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
    int32_t *cantidad_metricas_requerida)
{
    segmento_t segmento_sonda;
    metricas_proceso_t metrica_sonda;
    size_t cantidad_segmentos_size = 0;
    planificador_estado_t estado;
    int algoritmo;

    if (cantidad_segmentos_requerida != NULL) {
        *cantidad_segmentos_requerida = 0;
    }
    if (cantidad_metricas_requerida != NULL) {
        *cantidad_metricas_requerida = 0;
    }
    if (cantidad_segmentos_requerida == NULL
        || cantidad_metricas_requerida == NULL
        || procesos == NULL
        || algoritmo_id == NULL
        || (segmentos == NULL && capacidad_segmentos != 0)
        || (metricas == NULL && capacidad_metricas != 0)
        || cantidad_procesos <= 0
        || capacidad_segmentos < 0
        || capacidad_metricas < 0) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    int32_t cantidad_algoritmos = 0;
    const algoritmo_t *algoritmos = listar_algoritmos(&cantidad_algoritmos);
    for (algoritmo = 0; algoritmo < cantidad_algoritmos; algoritmo++) {
        if (strcmp(algoritmo_id, algoritmos[algoritmo].id) == 0) {
            break;
        }
    }
    if (algoritmo == cantidad_algoritmos) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
    if (algoritmo == 3 && quantum <= 0) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
#if SIZE_MAX < INT32_MAX
    if ((uintmax_t)(uint32_t)cantidad_procesos > (uintmax_t)SIZE_MAX
        || (uintmax_t)(uint32_t)capacidad_segmentos > (uintmax_t)SIZE_MAX
        || (uintmax_t)(uint32_t)capacidad_metricas > (uintmax_t)SIZE_MAX) {
        return PLANIFICADOR_ARGUMENTO_INVALIDO;
    }
#endif

    estado = ejecutar_seleccionado(
        algoritmo, quantum, procesos,
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

    estado = ejecutar_seleccionado(
        algoritmo, quantum, procesos,
        (size_t)cantidad_procesos,
        segmentos,
        (size_t)capacidad_segmentos,
        &cantidad_segmentos_size,
        metricas,
        (size_t)capacidad_metricas);
    return (int32_t)estado;
}
