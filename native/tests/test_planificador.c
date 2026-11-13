#include "planificador.h"

#include <stdio.h>

#define COMPROBAR(condicion) \
    do { \
        if (!(condicion)) { \
            fprintf(stderr, "Fallo en %s:%d: %s\n", __FILE__, __LINE__, #condicion); \
            return 1; \
        } \
    } while (0)

static int comprobar_segmento(
    const segmento_t *segmento,
    int32_t proceso_id,
    int64_t inicio,
    int64_t fin,
    int32_t es_ocioso)
{
    return segmento->proceso_id == proceso_id
        && segmento->inicio == inicio
        && segmento->fin == fin
        && segmento->es_ocioso == es_ocioso;
}

static int comprobar_metrica(
    const metricas_proceso_t *metrica,
    int32_t proceso_id,
    int64_t espera,
    int64_t retorno,
    int64_t respuesta)
{
    return metrica->proceso_id == proceso_id
        && metrica->espera == espera
        && metrica->retorno == retorno
        && metrica->respuesta == respuesta;
}

static void mostrar_resultados(
    const segmento_t *segmentos,
    size_t cantidad_segmentos,
    const metricas_proceso_t *metricas,
    size_t cantidad_procesos)
{
    puts("\n=== Diagrama de Gantt ===");

    for (size_t i = 0; i < cantidad_segmentos; i++) {
        if (segmentos[i].es_ocioso) {
            printf("CPU ociosa: %lld -> %lld\n",
                   (long long) segmentos[i].inicio,
                   (long long) segmentos[i].fin);
        } else {
            printf("P%d: %lld -> %lld\n",
                   segmentos[i].proceso_id,
                   (long long) segmentos[i].inicio,
                   (long long) segmentos[i].fin);
        }
    }

    puts("\n=== Metricas por proceso ===");
    puts("Proceso | Espera | Retorno | Respuesta");

    for (size_t i = 0; i < cantidad_procesos; i++) {
        printf("P%d      | %lld | %lld | %lld\n",
               metricas[i].proceso_id,
               (long long) metricas[i].espera,
               (long long) metricas[i].retorno,
               (long long) metricas[i].respuesta);
    }
}

static int probar_llegadas_desempates_y_ocio(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 3, .prioridad = 5 },
        { .id = 2, .llegada = 1, .rafaga = 2, .prioridad = 9 },
        { .id = 3, .llegada = 2, .rafaga = 1, .prioridad = 0 },
        { .id = 4, .llegada = 1, .rafaga = 1, .prioridad = 1 },
        { .id = 6, .llegada = 1, .rafaga = 1, .prioridad = 1 },
        { .id = 5, .llegada = 10, .rafaga = 2, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 1, .inicio = 0, .fin = 3, .es_ocioso = 0 },
        { .proceso_id = 4, .inicio = 3, .fin = 4, .es_ocioso = 0 },
        { .proceso_id = 6, .inicio = 4, .fin = 5, .es_ocioso = 0 },
        { .proceso_id = 2, .inicio = 5, .fin = 7, .es_ocioso = 0 },
        { .proceso_id = 3, .inicio = 7, .fin = 8, .es_ocioso = 0 },
        { .proceso_id = 0, .inicio = 8, .fin = 10, .es_ocioso = 1 },
        { .proceso_id = 5, .inicio = 10, .fin = 12, .es_ocioso = 0 }
    };
    const metricas_proceso_t metricas_esperadas[] = {
        { .proceso_id = 1, .espera = 0, .retorno = 3, .respuesta = 0 },
        { .proceso_id = 2, .espera = 4, .retorno = 6, .respuesta = 4 },
        { .proceso_id = 3, .espera = 5, .retorno = 6, .respuesta = 5 },
        { .proceso_id = 4, .espera = 2, .retorno = 3, .respuesta = 2 },
        { .proceso_id = 6, .espera = 3, .retorno = 4, .respuesta = 3 },
        { .proceso_id = 5, .espera = 0, .retorno = 2, .respuesta = 0 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_fcfs(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(esperados) / sizeof(esperados[0]));

    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    for (size_t i = 0; i < sizeof(procesos) / sizeof(procesos[0]); i++) {
        COMPROBAR(comprobar_metrica(
            &metricas[i],
            metricas_esperadas[i].proceso_id,
            metricas_esperadas[i].espera,
            metricas_esperadas[i].retorno,
            metricas_esperadas[i].respuesta));
    }
    mostrar_resultados(
        segmentos,
        cantidad_segmentos,
        metricas,
        sizeof(procesos) / sizeof(procesos[0]));
    return 0;
}

static int probar_un_proceso_y_ocio_inicial(void)
{
    const proceso_t proceso = {
        .id = 7,
        .llegada = 3,
        .rafaga = 2,
        .prioridad = 4
    };
    segmento_t segmentos[2];
    metricas_proceso_t metrica[1];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_fcfs(
        &proceso, 1, segmentos, 2, &cantidad_segmentos, metrica, 1) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == 2);
    COMPROBAR(comprobar_segmento(&segmentos[0], 0, 0, 3, 1));
    COMPROBAR(comprobar_segmento(&segmentos[1], 7, 3, 5, 0));
    COMPROBAR(comprobar_metrica(&metrica[0], 7, 0, 2, 0));
    return 0;
}

static int probar_validacion_y_capacidad(void)
{
    proceso_t proceso = { .id = 1, .llegada = 0, .rafaga = 1, .prioridad = 0 };
    segmento_t segmento = { .proceso_id = 99, .inicio = 99, .fin = 99, .es_ocioso = 0 };
    metricas_proceso_t metrica = { .proceso_id = 99, .espera = 99, .retorno = 99, .respuesta = 99 };
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_fcfs(
        &proceso, 1, &segmento, 0, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
    COMPROBAR(cantidad_segmentos == 1);
    COMPROBAR(segmento.proceso_id == 99 && metrica.proceso_id == 99);

    proceso.rafaga = 0;
    COMPROBAR(planificador_fcfs(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);

    return 0;
}

static int probar_desempates_fcfs(void)
{
    const proceso_t procesos[] = {
        { .id = 20, .llegada = 0, .rafaga = 1, .prioridad = 8 },
        { .id = 30, .llegada = 0, .rafaga = 1, .prioridad = 2 },
        { .id = 40, .llegada = 0, .rafaga = 1, .prioridad = 2 },
        { .id = 10, .llegada = 0, .rafaga = 1, .prioridad = 9 }
    };
    const int32_t orden_esperado[] = { 30, 40, 20, 10 };
    segmento_t segmentos[sizeof(procesos) / sizeof(procesos[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_fcfs(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(orden_esperado) / sizeof(orden_esperado[0]));

    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(segmentos[i].proceso_id == orden_esperado[i]);
    }
    return 0;
}

static int probar_errores_no_escriben_resultados(void)
{
    proceso_t proceso = { .id = 1, .llegada = 0, .rafaga = 1, .prioridad = 0 };
    segmento_t segmentos[2] = {
        { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 },
        { .proceso_id = 81, .inicio = 82, .fin = 83, .es_ocioso = 84 }
    };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 99;

    COMPROBAR(planificador_fcfs(
        &proceso, 1, segmentos, 2, NULL, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(segmentos[0].proceso_id == 91 && metrica.proceso_id == 71);

    proceso.llegada = -1;
    COMPROBAR(planificador_fcfs(
        &proceso, 1, segmentos, 2, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmentos[0].proceso_id == 91
        && segmentos[0].inicio == 92
        && segmentos[0].fin == 93
        && segmentos[0].es_ocioso == 94);
    COMPROBAR(segmentos[1].proceso_id == 81
        && segmentos[1].inicio == 82
        && segmentos[1].fin == 83
        && segmentos[1].es_ocioso == 84);
    COMPROBAR(metrica.proceso_id == 71
        && metrica.espera == 72
        && metrica.retorno == 73
        && metrica.respuesta == 74);

    proceso.llegada = 0;
    COMPROBAR(planificador_fcfs(
        NULL, 1, segmentos, 2, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmentos[0].proceso_id == 91 && metrica.proceso_id == 71);
    return 0;
}

static int probar_exito_v1_equivale_a_fcfs(void)
{
    const proceso_t procesos[] = {
        { .id = 11, .llegada = 0, .rafaga = 2, .prioridad = 0 },
        { .id = 12, .llegada = 0, .rafaga = 1, .prioridad = 2 },
        { .id = 13, .llegada = 5, .rafaga = 2, .prioridad = 1 }
    };
    segmento_t segmentos_fcfs[4];
    segmento_t segmentos_v1[4];
    metricas_proceso_t metricas_fcfs[3];
    metricas_proceso_t metricas_v1[3];
    size_t cantidad_segmentos_fcfs = 0;
    int32_t cantidad_segmentos_v1 = -1;
    int32_t cantidad_metricas_v1 = -1;
    const size_t cantidad_procesos = sizeof(procesos) / sizeof(procesos[0]);

    COMPROBAR(planificador_fcfs(
        procesos,
        cantidad_procesos,
        segmentos_fcfs,
        sizeof(segmentos_fcfs) / sizeof(segmentos_fcfs[0]),
        &cantidad_segmentos_fcfs,
        metricas_fcfs,
        sizeof(metricas_fcfs) / sizeof(metricas_fcfs[0])) == PLANIFICADOR_OK);

    COMPROBAR(planificador_ejecutar_v1(
        "fcfs", procesos, (int32_t)cantidad_procesos, 0,
        segmentos_v1,
        (int32_t)(sizeof(segmentos_v1) / sizeof(segmentos_v1[0])),
        &cantidad_segmentos_v1,
        metricas_v1,
        (int32_t)(sizeof(metricas_v1) / sizeof(metricas_v1[0])),
        &cantidad_metricas_v1) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos_v1 == (int32_t)cantidad_segmentos_fcfs);
    COMPROBAR(cantidad_metricas_v1 == (int32_t)cantidad_procesos);

    for (size_t i = 0; i < cantidad_segmentos_fcfs; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos_v1[i],
            segmentos_fcfs[i].proceso_id,
            segmentos_fcfs[i].inicio,
            segmentos_fcfs[i].fin,
            segmentos_fcfs[i].es_ocioso));
    }
    for (size_t i = 0; i < cantidad_procesos; i++) {
        COMPROBAR(comprobar_metrica(
            &metricas_v1[i],
            metricas_fcfs[i].proceso_id,
            metricas_fcfs[i].espera,
            metricas_fcfs[i].retorno,
            metricas_fcfs[i].respuesta));
    }
    return 0;
}

static int probar_capacidades_insuficientes_v1(void)
{
    const proceso_t proceso = {
        .id = 1,
        .llegada = 3,
        .rafaga = 2,
        .prioridad = 0
    };
    segmento_t segmentos[2];
    metricas_proceso_t metricas[1];
    const struct {
        int32_t capacidad_segmentos;
        int32_t capacidad_metricas;
    } casos[] = {
        { .capacidad_segmentos = 1, .capacidad_metricas = 1 },
        { .capacidad_segmentos = 2, .capacidad_metricas = 0 },
        { .capacidad_segmentos = 1, .capacidad_metricas = 0 }
    };

    for (size_t i = 0; i < sizeof(casos) / sizeof(casos[0]); i++) {
        segmentos[0] = (segmento_t) {
            .proceso_id = 99,
            .inicio = 99,
            .fin = 99,
            .es_ocioso = 99
        };
        segmentos[1] = (segmento_t) {
            .proceso_id = 98,
            .inicio = 98,
            .fin = 98,
            .es_ocioso = 98
        };
        metricas[0] = (metricas_proceso_t) {
            .proceso_id = 97,
            .espera = 97,
            .retorno = 97,
            .respuesta = 97
        };

        int32_t cantidad_segmentos_requerida = -1;
        int32_t cantidad_metricas_requerida = -1;

        COMPROBAR(planificador_ejecutar_v1(
        "fcfs", &proceso, 1, 0,
            segmentos,
            casos[i].capacidad_segmentos,
            &cantidad_segmentos_requerida,
            metricas,
            casos[i].capacidad_metricas,
            &cantidad_metricas_requerida)
            == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
        COMPROBAR(cantidad_segmentos_requerida == 2);
        COMPROBAR(cantidad_metricas_requerida == 1);
        COMPROBAR(segmentos[0].proceso_id == 99
            && segmentos[0].inicio == 99
            && segmentos[0].fin == 99
            && segmentos[0].es_ocioso == 99);
        COMPROBAR(segmentos[1].proceso_id == 98
            && segmentos[1].inicio == 98
            && segmentos[1].fin == 98
            && segmentos[1].es_ocioso == 98);
        COMPROBAR(metricas[0].proceso_id == 97
            && metricas[0].espera == 97
            && metricas[0].retorno == 97
            && metricas[0].respuesta == 97);
    }
    return 0;
}

static int probar_desbordamiento_v1(void)
{
    const proceso_t proceso = {
        .id = 1,
        .llegada = INT64_MAX,
        .rafaga = 1,
        .prioridad = 0
    };
    segmento_t segmento = {
        .proceso_id = 99,
        .inicio = 99,
        .fin = 99,
        .es_ocioso = 99
    };
    metricas_proceso_t metrica = {
        .proceso_id = 98,
        .espera = 98,
        .retorno = 98,
        .respuesta = 98
    };
    int32_t cantidad_segmentos_requerida = -1;
    int32_t cantidad_metricas_requerida = -1;

    COMPROBAR(planificador_ejecutar_v1(
        "fcfs", &proceso, 1, 0,
        &segmento,
        1,
        &cantidad_segmentos_requerida,
        &metrica,
        1,
        &cantidad_metricas_requerida)
        == PLANIFICADOR_DESBORDAMIENTO_TIEMPO);
    COMPROBAR(cantidad_segmentos_requerida == 0);
    COMPROBAR(cantidad_metricas_requerida == 0);
    COMPROBAR(segmento.proceso_id == 99
        && segmento.inicio == 99
        && segmento.fin == 99
        && segmento.es_ocioso == 99);
    COMPROBAR(metrica.proceso_id == 98
        && metrica.espera == 98
        && metrica.retorno == 98
        && metrica.respuesta == 98);
    return 0;
}

static int probar_argumentos_invalidos_v1(void)
{
    const proceso_t proceso = {
        .id = 1,
        .llegada = 0,
        .rafaga = 1,
        .prioridad = 0
    };
    segmento_t segmento = {
        .proceso_id = 99,
        .inicio = 99,
        .fin = 99,
        .es_ocioso = 99
    };
    metricas_proceso_t metrica = {
        .proceso_id = 98,
        .espera = 98,
        .retorno = 98,
        .respuesta = 98
    };
    int32_t cantidad_segmentos_requerida = -1;
    int32_t cantidad_metricas_requerida = -1;

    COMPROBAR(planificador_ejecutar_v1(
        "fcfs", &proceso, 1, 0,
        &segmento,
        1,
        NULL,
        &metrica,
        1,
        &cantidad_metricas_requerida) == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_metricas_requerida == 0);
    COMPROBAR(segmento.proceso_id == 99 && metrica.proceso_id == 98);

    cantidad_segmentos_requerida = -1;
    cantidad_metricas_requerida = -1;
    COMPROBAR(planificador_ejecutar_v1(
        "fcfs", NULL, 1, 0,
        &segmento,
        1,
        &cantidad_segmentos_requerida,
        &metrica,
        1,
        &cantidad_metricas_requerida) == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos_requerida == 0);
    COMPROBAR(cantidad_metricas_requerida == 0);
    COMPROBAR(segmento.proceso_id == 99 && metrica.proceso_id == 98);
    return 0;
}

static int probar_sjf_llegadas_ocio_no_expropiacion_y_metricas(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 2, .rafaga = 6, .prioridad = 0 },
        { .id = 2, .llegada = 3, .rafaga = 2, .prioridad = 0 },
        { .id = 3, .llegada = 4, .rafaga = 1, .prioridad = 0 },
        { .id = 4, .llegada = 15, .rafaga = 2, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 0, .inicio = 0, .fin = 2, .es_ocioso = 1 },
        { .proceso_id = 1, .inicio = 2, .fin = 8, .es_ocioso = 0 },
        { .proceso_id = 3, .inicio = 8, .fin = 9, .es_ocioso = 0 },
        { .proceso_id = 2, .inicio = 9, .fin = 11, .es_ocioso = 0 },
        { .proceso_id = 0, .inicio = 11, .fin = 15, .es_ocioso = 1 },
        { .proceso_id = 4, .inicio = 15, .fin = 17, .es_ocioso = 0 }
    };
    const metricas_proceso_t metricas_esperadas[] = {
        { .proceso_id = 1, .espera = 0, .retorno = 6, .respuesta = 0 },
        { .proceso_id = 2, .espera = 6, .retorno = 8, .respuesta = 6 },
        { .proceso_id = 3, .espera = 4, .retorno = 5, .respuesta = 4 },
        { .proceso_id = 4, .espera = 0, .retorno = 2, .respuesta = 0 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_sjf(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(esperados) / sizeof(esperados[0]));

    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    for (size_t i = 0; i < sizeof(procesos) / sizeof(procesos[0]); i++) {
        COMPROBAR(comprobar_metrica(
            &metricas[i],
            metricas_esperadas[i].proceso_id,
            metricas_esperadas[i].espera,
            metricas_esperadas[i].retorno,
            metricas_esperadas[i].respuesta));
    }
    return 0;
}

static int probar_sjf_desempates_provisionales(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 4, .prioridad = 0 },
        { .id = 2, .llegada = 1, .rafaga = 2, .prioridad = 9 },
        { .id = 3, .llegada = 1, .rafaga = 2, .prioridad = 2 },
        { .id = 4, .llegada = 1, .rafaga = 2, .prioridad = 2 },
        { .id = 5, .llegada = 2, .rafaga = 2, .prioridad = 1 }
    };
    const int32_t orden_esperado[] = { 1, 3, 4, 2, 5 };
    segmento_t segmentos[sizeof(procesos) / sizeof(procesos[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_sjf(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(orden_esperado) / sizeof(orden_esperado[0]));
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(segmentos[i].proceso_id == orden_esperado[i]);
    }
    return 0;
}

static int probar_sjf_capacidad_insuficiente_sin_escritura(void)
{
    const proceso_t proceso = {
        .id = 7,
        .llegada = 3,
        .rafaga = 2,
        .prioridad = 0
    };
    segmento_t segmentos[2] = {
        { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 },
        { .proceso_id = 81, .inicio = 82, .fin = 83, .es_ocioso = 84 }
    };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_sjf(
        &proceso, 1, segmentos, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
    COMPROBAR(cantidad_segmentos == 2);
    COMPROBAR(segmentos[0].proceso_id == 91 && segmentos[1].proceso_id == 81);
    COMPROBAR(metrica.proceso_id == 71 && metrica.espera == 72);

    cantidad_segmentos = 0;
    COMPROBAR(planificador_sjf(
        &proceso, 1, segmentos, 2, &cantidad_segmentos, &metrica, 0)
        == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
    COMPROBAR(cantidad_segmentos == 2);
    COMPROBAR(segmentos[0].proceso_id == 91 && segmentos[1].proceso_id == 81);
    COMPROBAR(metrica.proceso_id == 71 && metrica.espera == 72);
    return 0;
}

static int probar_sjf_argumentos_invalidos(void)
{
    proceso_t proceso = { .id = 1, .llegada = 0, .rafaga = 1, .prioridad = 0 };
    segmento_t segmento = { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 99;

    COMPROBAR(planificador_sjf(
        &proceso, 0, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    COMPROBAR(planificador_sjf(
        &proceso, 1, &segmento, 1, NULL, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(segmento.proceso_id == 91 && metrica.proceso_id == 71);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_sjf(
        NULL, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_sjf(
        &proceso, 1, NULL, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_sjf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, NULL, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    proceso.llegada = -1;
    cantidad_segmentos = 99;
    COMPROBAR(planificador_sjf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    proceso.llegada = 0;
    proceso.rafaga = 0;
    cantidad_segmentos = 99;
    COMPROBAR(planificador_sjf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmento.proceso_id == 91 && metrica.proceso_id == 71);
    return 0;
}

static int probar_sjf_desbordamiento_sin_escritura(void)
{
    const proceso_t proceso = {
        .id = 5,
        .llegada = INT64_MAX,
        .rafaga = 1,
        .prioridad = 0
    };
    segmento_t segmento = { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 99;

    COMPROBAR(planificador_sjf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_DESBORDAMIENTO_TIEMPO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmento.proceso_id == 91
        && segmento.inicio == 92
        && segmento.fin == 93
        && segmento.es_ocioso == 94);
    COMPROBAR(metrica.proceso_id == 71
        && metrica.espera == 72
        && metrica.retorno == 73
        && metrica.respuesta == 74);
    return 0;
}

static int probar_srtf_expropiacion_y_metricas(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 8, .prioridad = 0 },
        { .id = 2, .llegada = 2, .rafaga = 3, .prioridad = 0 },
        { .id = 3, .llegada = 3, .rafaga = 10, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 1, .inicio = 0, .fin = 2, .es_ocioso = 0 },
        { .proceso_id = 2, .inicio = 2, .fin = 5, .es_ocioso = 0 },
        { .proceso_id = 1, .inicio = 5, .fin = 11, .es_ocioso = 0 },
        { .proceso_id = 3, .inicio = 11, .fin = 21, .es_ocioso = 0 }
    };
    const metricas_proceso_t metricas_esperadas[] = {
        { .proceso_id = 1, .espera = 3, .retorno = 11, .respuesta = 0 },
        { .proceso_id = 2, .espera = 0, .retorno = 3, .respuesta = 0 },
        { .proceso_id = 3, .espera = 8, .retorno = 18, .respuesta = 8 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_srtf(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(esperados) / sizeof(esperados[0]));
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    for (size_t i = 0; i < sizeof(procesos) / sizeof(procesos[0]); i++) {
        COMPROBAR(comprobar_metrica(
            &metricas[i],
            metricas_esperadas[i].proceso_id,
            metricas_esperadas[i].espera,
            metricas_esperadas[i].retorno,
            metricas_esperadas[i].respuesta));
    }
    return 0;
}

static int probar_srtf_sin_expropiacion_ni_segmentos_redundantes(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 4, .prioridad = 0 },
        { .id = 2, .llegada = 1, .rafaga = 5, .prioridad = 0 },
        { .id = 3, .llegada = 1, .rafaga = 3, .prioridad = 0 }
    };
    const int32_t orden_esperado[] = { 1, 3, 2 };
    const int64_t inicios_esperados[] = { 0, 4, 7 };
    const int64_t fines_esperados[] = { 4, 7, 12 };
    segmento_t segmentos[sizeof(procesos) / sizeof(procesos[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_srtf(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(orden_esperado) / sizeof(orden_esperado[0]));
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            orden_esperado[i],
            inicios_esperados[i],
            fines_esperados[i],
            0));
    }
    return 0;
}

static int probar_srtf_llegadas_simultaneas_y_empates(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 5, .prioridad = 0 },
        { .id = 2, .llegada = 0, .rafaga = 2, .prioridad = 4 },
        { .id = 3, .llegada = 0, .rafaga = 2, .prioridad = 1 }
    };
    const int32_t orden_esperado[] = { 3, 2, 1 };
    const int64_t inicios_esperados[] = { 0, 2, 4 };
    const int64_t fines_esperados[] = { 2, 4, 9 };
    segmento_t segmentos[sizeof(procesos) / sizeof(procesos[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_srtf(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(orden_esperado) / sizeof(orden_esperado[0]));
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            orden_esperado[i],
            inicios_esperados[i],
            fines_esperados[i],
            0));
    }
    return 0;
}

static int probar_srtf_empate_del_proceso_actual(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 4, .prioridad = 9 },
        { .id = 2, .llegada = 1, .rafaga = 3, .prioridad = 0 }
    };
    segmento_t segmentos[2];
    metricas_proceso_t metricas[2];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_srtf(
        procesos, 2, segmentos, 2, &cantidad_segmentos, metricas, 2)
        == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == 2);
    COMPROBAR(comprobar_segmento(&segmentos[0], 1, 0, 4, 0));
    COMPROBAR(comprobar_segmento(&segmentos[1], 2, 4, 7, 0));
    return 0;
}

static int probar_srtf_ocio_inicial_e_intermedio(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 3, .rafaga = 2, .prioridad = 0 },
        { .id = 2, .llegada = 10, .rafaga = 1, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 0, .inicio = 0, .fin = 3, .es_ocioso = 1 },
        { .proceso_id = 1, .inicio = 3, .fin = 5, .es_ocioso = 0 },
        { .proceso_id = 0, .inicio = 5, .fin = 10, .es_ocioso = 1 },
        { .proceso_id = 2, .inicio = 10, .fin = 11, .es_ocioso = 0 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_srtf(
        procesos,
        sizeof(procesos) / sizeof(procesos[0]),
        segmentos,
        sizeof(segmentos) / sizeof(segmentos[0]),
        &cantidad_segmentos,
        metricas,
        sizeof(metricas) / sizeof(metricas[0])) == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(esperados) / sizeof(esperados[0]));
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    return 0;
}

static int probar_srtf_capacidad_insuficiente_sin_escritura(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 5, .prioridad = 0 },
        { .id = 2, .llegada = 1, .rafaga = 1, .prioridad = 0 }
    };
    segmento_t segmentos[3] = {
        { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 },
        { .proceso_id = 81, .inicio = 82, .fin = 83, .es_ocioso = 84 },
        { .proceso_id = 71, .inicio = 72, .fin = 73, .es_ocioso = 74 }
    };
    metricas_proceso_t metricas[2] = {
        { .proceso_id = 61, .espera = 62, .retorno = 63, .respuesta = 64 },
        { .proceso_id = 51, .espera = 52, .retorno = 53, .respuesta = 54 }
    };
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_srtf(
        procesos, 2, segmentos, 2, &cantidad_segmentos, metricas, 2)
        == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
    COMPROBAR(cantidad_segmentos == 3);
    COMPROBAR(segmentos[0].proceso_id == 91
        && segmentos[1].proceso_id == 81
        && segmentos[2].proceso_id == 71);
    COMPROBAR(metricas[0].proceso_id == 61 && metricas[1].proceso_id == 51);

    cantidad_segmentos = 0;
    COMPROBAR(planificador_srtf(
        procesos, 2, segmentos, 3, &cantidad_segmentos, metricas, 1)
        == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
    COMPROBAR(cantidad_segmentos == 3);
    COMPROBAR(segmentos[0].proceso_id == 91
        && segmentos[1].proceso_id == 81
        && segmentos[2].proceso_id == 71);
    COMPROBAR(metricas[0].proceso_id == 61 && metricas[1].proceso_id == 51);
    return 0;
}

static int probar_srtf_argumentos_invalidos(void)
{
    proceso_t proceso = { .id = 1, .llegada = 0, .rafaga = 1, .prioridad = 0 };
    segmento_t segmento = { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 99;

    COMPROBAR(planificador_srtf(
        &proceso, 0, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    COMPROBAR(planificador_srtf(
        &proceso, 1, &segmento, 1, NULL, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(segmento.proceso_id == 91 && metrica.proceso_id == 71);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_srtf(
        NULL, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_srtf(
        &proceso, 1, NULL, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_srtf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, NULL, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    proceso.llegada = -1;
    cantidad_segmentos = 99;
    COMPROBAR(planificador_srtf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    proceso.llegada = 0;
    proceso.rafaga = 0;
    cantidad_segmentos = 99;
    COMPROBAR(planificador_srtf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmento.proceso_id == 91 && metrica.proceso_id == 71);
    return 0;
}

static int probar_srtf_desbordamiento_sin_escritura(void)
{
    const proceso_t proceso = {
        .id = 5,
        .llegada = INT64_MAX,
        .rafaga = 1,
        .prioridad = 0
    };
    segmento_t segmento = { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 99;

    COMPROBAR(planificador_srtf(
        &proceso, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_DESBORDAMIENTO_TIEMPO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmento.proceso_id == 91
        && segmento.inicio == 92
        && segmento.fin == 93
        && segmento.es_ocioso == 94);
    COMPROBAR(metrica.proceso_id == 71
        && metrica.espera == 72
        && metrica.retorno == 73
        && metrica.respuesta == 74);
    return 0;
}

static int probar_rr_basico_y_metricas(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 5, .prioridad = 0 },
        { .id = 2, .llegada = 0, .rafaga = 3, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 1, .inicio = 0, .fin = 2, .es_ocioso = 0 },
        { .proceso_id = 2, .inicio = 2, .fin = 4, .es_ocioso = 0 },
        { .proceso_id = 1, .inicio = 4, .fin = 6, .es_ocioso = 0 },
        { .proceso_id = 2, .inicio = 6, .fin = 7, .es_ocioso = 0 },
        { .proceso_id = 1, .inicio = 7, .fin = 8, .es_ocioso = 0 }
    };
    const metricas_proceso_t metricas_esperadas[] = {
        { .proceso_id = 1, .espera = 3, .retorno = 8, .respuesta = 0 },
        { .proceso_id = 2, .espera = 4, .retorno = 7, .respuesta = 2 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_rr(
        procesos, 2, 2, segmentos, 5, &cantidad_segmentos, metricas, 2)
        == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == 5);
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    for (size_t i = 0; i < 2; i++) {
        COMPROBAR(comprobar_metrica(
            &metricas[i],
            metricas_esperadas[i].proceso_id,
            metricas_esperadas[i].espera,
            metricas_esperadas[i].retorno,
            metricas_esperadas[i].respuesta));
    }
    return 0;
}

static int probar_rr_llegadas_simultaneas_al_vencer_quantum(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 5, .prioridad = 0 },
        { .id = 2, .llegada = 2, .rafaga = 2, .prioridad = 0 },
        { .id = 3, .llegada = 2, .rafaga = 1, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 1, .inicio = 0, .fin = 2, .es_ocioso = 0 },
        { .proceso_id = 2, .inicio = 2, .fin = 4, .es_ocioso = 0 },
        { .proceso_id = 3, .inicio = 4, .fin = 5, .es_ocioso = 0 },
        { .proceso_id = 1, .inicio = 5, .fin = 8, .es_ocioso = 0 }
    };
    const metricas_proceso_t metricas_esperadas[] = {
        { .proceso_id = 1, .espera = 3, .retorno = 8, .respuesta = 0 },
        { .proceso_id = 2, .espera = 0, .retorno = 2, .respuesta = 0 },
        { .proceso_id = 3, .espera = 2, .retorno = 3, .respuesta = 2 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_rr(
        procesos, 3, 2, segmentos, 4, &cantidad_segmentos, metricas, 3)
        == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == 4);
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    for (size_t i = 0; i < 3; i++) {
        COMPROBAR(comprobar_metrica(
            &metricas[i],
            metricas_esperadas[i].proceso_id,
            metricas_esperadas[i].espera,
            metricas_esperadas[i].retorno,
            metricas_esperadas[i].respuesta));
    }
    return 0;
}

static int probar_rr_llegada_durante_quantum(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 5, .prioridad = 0 },
        { .id = 2, .llegada = 1, .rafaga = 1, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 1, .inicio = 0, .fin = 3, .es_ocioso = 0 },
        { .proceso_id = 2, .inicio = 3, .fin = 4, .es_ocioso = 0 },
        { .proceso_id = 1, .inicio = 4, .fin = 6, .es_ocioso = 0 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_rr(
        procesos, 2, 3, segmentos, 3, &cantidad_segmentos, metricas, 2)
        == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == 3);
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    return 0;
}

static int probar_rr_ocio_y_fusion_de_turnos(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 3, .rafaga = 3, .prioridad = 0 },
        { .id = 2, .llegada = 12, .rafaga = 1, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 0, .inicio = 0, .fin = 3, .es_ocioso = 1 },
        { .proceso_id = 1, .inicio = 3, .fin = 6, .es_ocioso = 0 },
        { .proceso_id = 0, .inicio = 6, .fin = 12, .es_ocioso = 1 },
        { .proceso_id = 2, .inicio = 12, .fin = 13, .es_ocioso = 0 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_rr(
        procesos, 2, 2, segmentos, 4, &cantidad_segmentos, metricas, 2)
        == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == 4);
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    return 0;
}

static int probar_rr_capacidad_insuficiente_sin_escritura(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 5, .prioridad = 0 },
        { .id = 2, .llegada = 0, .rafaga = 3, .prioridad = 0 }
    };
    segmento_t segmentos[5] = {
        { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 },
        { .proceso_id = 81, .inicio = 82, .fin = 83, .es_ocioso = 84 },
        { .proceso_id = 71, .inicio = 72, .fin = 73, .es_ocioso = 74 },
        { .proceso_id = 61, .inicio = 62, .fin = 63, .es_ocioso = 64 },
        { .proceso_id = 51, .inicio = 52, .fin = 53, .es_ocioso = 54 }
    };
    metricas_proceso_t metricas[2] = {
        { .proceso_id = 41, .espera = 42, .retorno = 43, .respuesta = 44 },
        { .proceso_id = 31, .espera = 32, .retorno = 33, .respuesta = 34 }
    };
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_rr(
        procesos, 2, 2, segmentos, 4, &cantidad_segmentos, metricas, 2)
        == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
    COMPROBAR(cantidad_segmentos == 5);
    COMPROBAR(segmentos[0].proceso_id == 91
        && segmentos[1].proceso_id == 81
        && segmentos[2].proceso_id == 71
        && segmentos[3].proceso_id == 61
        && segmentos[4].proceso_id == 51);
    COMPROBAR(metricas[0].proceso_id == 41 && metricas[1].proceso_id == 31);

    cantidad_segmentos = 0;
    COMPROBAR(planificador_rr(
        procesos, 2, 2, segmentos, 5, &cantidad_segmentos, metricas, 1)
        == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
    COMPROBAR(cantidad_segmentos == 5);
    COMPROBAR(segmentos[0].proceso_id == 91
        && segmentos[1].proceso_id == 81
        && segmentos[2].proceso_id == 71
        && segmentos[3].proceso_id == 61
        && segmentos[4].proceso_id == 51);
    COMPROBAR(metricas[0].proceso_id == 41 && metricas[1].proceso_id == 31);
    return 0;
}

static int probar_rr_quantum_y_argumentos_invalidos(void)
{
    proceso_t proceso = { .id = 1, .llegada = 0, .rafaga = 1, .prioridad = 0 };
    segmento_t segmento = { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 99;

    COMPROBAR(planificador_rr(
        &proceso, 1, 0, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_rr(
        &proceso, 1, -1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    COMPROBAR(planificador_rr(
        &proceso, 1, 1, &segmento, 1, NULL, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(segmento.proceso_id == 91 && metrica.proceso_id == 71);

    cantidad_segmentos = 99;
    COMPROBAR(planificador_rr(
        NULL, 1, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    cantidad_segmentos = 99;
    COMPROBAR(planificador_rr(
        &proceso, 1, 1, NULL, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    cantidad_segmentos = 99;
    COMPROBAR(planificador_rr(
        &proceso, 1, 1, &segmento, 1, &cantidad_segmentos, NULL, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);

    proceso.llegada = -1;
    cantidad_segmentos = 99;
    COMPROBAR(planificador_rr(
        &proceso, 1, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    proceso.llegada = 0;
    proceso.rafaga = 0;
    cantidad_segmentos = 99;
    COMPROBAR(planificador_rr(
        &proceso, 1, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_ARGUMENTO_INVALIDO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmento.proceso_id == 91 && metrica.proceso_id == 71);
    return 0;
}

static int probar_rr_desbordamiento_sin_escritura(void)
{
    const proceso_t proceso = {
        .id = 5,
        .llegada = INT64_MAX,
        .rafaga = 1,
        .prioridad = 0
    };
    segmento_t segmento = { .proceso_id = 91, .inicio = 92, .fin = 93, .es_ocioso = 94 };
    metricas_proceso_t metrica = {
        .proceso_id = 71,
        .espera = 72,
        .retorno = 73,
        .respuesta = 74
    };
    size_t cantidad_segmentos = 99;

    COMPROBAR(planificador_rr(
        &proceso, 1, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_DESBORDAMIENTO_TIEMPO);
    COMPROBAR(cantidad_segmentos == 0);
    COMPROBAR(segmento.proceso_id == 91
        && segmento.inicio == 92
        && segmento.fin == 93
        && segmento.es_ocioso == 94);
    COMPROBAR(metrica.proceso_id == 71
        && metrica.espera == 72
        && metrica.retorno == 73
        && metrica.respuesta == 74);
    return 0;
}

static int probar_rr_rafaga_grande_sin_competencia(void)
{
    const proceso_t proceso = {
        .id = 7,
        .llegada = 0,
        .rafaga = 1000000000,
        .prioridad = 0
    };
    segmento_t segmento;
    metricas_proceso_t metrica;
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_rr(
        &proceso, 1, 1, &segmento, 1, &cantidad_segmentos, &metrica, 1)
        == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == 1);
    COMPROBAR(comprobar_segmento(&segmento, 7, 0, 1000000000, 0));
    COMPROBAR(comprobar_metrica(&metrica, 7, 0, 1000000000, 0));
    return 0;
}

static int probar_rr_salto_hasta_finalizacion_antes_de_otra_llegada(void)
{
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 0, .rafaga = 1000000000, .prioridad = 0 },
        { .id = 2, .llegada = 1000000005, .rafaga = 1, .prioridad = 0 }
    };
    const segmento_t esperados[] = {
        { .proceso_id = 1, .inicio = 0, .fin = 1000000000, .es_ocioso = 0 },
        { .proceso_id = 0, .inicio = 1000000000, .fin = 1000000005, .es_ocioso = 1 },
        { .proceso_id = 2, .inicio = 1000000005, .fin = 1000000006, .es_ocioso = 0 }
    };
    const metricas_proceso_t metricas_esperadas[] = {
        { .proceso_id = 1, .espera = 0, .retorno = 1000000000, .respuesta = 0 },
        { .proceso_id = 2, .espera = 0, .retorno = 1, .respuesta = 0 }
    };
    segmento_t segmentos[sizeof(esperados) / sizeof(esperados[0])];
    metricas_proceso_t metricas[sizeof(procesos) / sizeof(procesos[0])];
    size_t cantidad_segmentos = 0;

    COMPROBAR(planificador_rr(
        procesos, 2, 1, segmentos, 3, &cantidad_segmentos, metricas, 2)
        == PLANIFICADOR_OK);
    COMPROBAR(cantidad_segmentos == sizeof(esperados) / sizeof(esperados[0]));
    for (size_t i = 0; i < cantidad_segmentos; i++) {
        COMPROBAR(comprobar_segmento(
            &segmentos[i],
            esperados[i].proceso_id,
            esperados[i].inicio,
            esperados[i].fin,
            esperados[i].es_ocioso));
    }
    for (size_t i = 0; i < sizeof(procesos) / sizeof(procesos[0]); i++) {
        COMPROBAR(comprobar_metrica(
            &metricas[i],
            metricas_esperadas[i].proceso_id,
            metricas_esperadas[i].espera,
            metricas_esperadas[i].retorno,
            metricas_esperadas[i].respuesta));
    }
    return 0;
}


static int probar_interfaz_comun(void)
{
    const char *ids[] = { "fcfs", "sjf", "srtf", "rr" };
    const proceso_t procesos[] = {
        { .id = 1, .llegada = 1, .rafaga = 7, .prioridad = 1 },
        { .id = 2, .llegada = 3, .rafaga = 2, .prioridad = 0 },
        { .id = 3, .llegada = 12, .rafaga = 1, .prioridad = 0 }
    };
    for (size_t a = 0; a < 4; a++) {
        segmento_t directos[32], comunes[32];
        metricas_proceso_t md[3], mc[3];
        size_t nd = 0;
        int32_t ns = -1, nm = -1;
        planificador_estado_t estado;
        if (a == 0) estado = planificador_fcfs(procesos, 3, directos, 32, &nd, md, 3);
        else if (a == 1) estado = planificador_sjf(procesos, 3, directos, 32, &nd, md, 3);
        else if (a == 2) estado = planificador_srtf(procesos, 3, directos, 32, &nd, md, 3);
        else estado = planificador_rr(procesos, 3, 2, directos, 32, &nd, md, 3);
        COMPROBAR(estado == PLANIFICADOR_OK);
        COMPROBAR(planificador_ejecutar_v1(ids[a], procesos, 3, 2,
            NULL, 0, &ns, NULL, 0, &nm) == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
        COMPROBAR(ns == (int32_t)nd && nm == 3);
        COMPROBAR(planificador_ejecutar_v1(ids[a], procesos, 3, 2,
            comunes, ns, &ns, mc, nm, &nm) == PLANIFICADOR_OK);
        for (size_t i = 0; i < nd; i++) {
            COMPROBAR(comprobar_segmento(&comunes[i], directos[i].proceso_id,
                directos[i].inicio, directos[i].fin, directos[i].es_ocioso));
        }
        for (size_t i = 0; i < 3; i++) {
            COMPROBAR(comprobar_metrica(&mc[i], md[i].proceso_id,
                md[i].espera, md[i].retorno, md[i].respuesta));
        }
        for (int falta = 0; falta < 3; falta++) {
            comunes[0] = (segmento_t){99, 99, 99, 99};
            mc[0] = (metricas_proceso_t){98, 98, 98, 98};
            COMPROBAR(planificador_ejecutar_v1(ids[a], procesos, 3, 2,
                comunes, falta == 1 ? ns : ns - 1, &ns,
                mc, falta == 0 ? 3 : 2, &nm) == PLANIFICADOR_CAPACIDAD_INSUFICIENTE);
            COMPROBAR(ns == (int32_t)nd && nm == 3);
            COMPROBAR(comprobar_segmento(&comunes[0], 99, 99, 99, 99));
            COMPROBAR(comprobar_metrica(&mc[0], 98, 98, 98, 98));
        }
        proceso_t invalido = procesos[0];
        for (int error = 0; error < 3; error++) {
            invalido = procesos[0];
            if (error == 0) invalido.llegada = -1;
            if (error == 1) invalido.rafaga = 0;
            if (error == 2) invalido.llegada = INT64_MAX;
            COMPROBAR(planificador_ejecutar_v1(ids[a], &invalido, 1, 2,
                NULL, 0, &ns, NULL, 0, &nm) ==
                (error == 2 ? PLANIFICADOR_DESBORDAMIENTO_TIEMPO : PLANIFICADOR_ARGUMENTO_INVALIDO));
            COMPROBAR(ns == 0 && nm == 0);
        }
    }
    return 0;
}

static int probar_interfaz_comun_argumentos(void)
{
    const proceso_t p = {1, 0, 1, 0};
    segmento_t s = {99, 99, 99, 99};
    metricas_proceso_t m = {98, 98, 98, 98};
    int32_t ns, nm;
    const char *invalidos[] = {NULL, "", "FCFS", "desconocido"};
    for (size_t i = 0; i < 4; i++) {
        COMPROBAR(planificador_ejecutar_v1(invalidos[i], &p, 1, 0,
            &s, 1, &ns, &m, 1, &nm) == PLANIFICADOR_ARGUMENTO_INVALIDO);
        COMPROBAR(ns == 0 && nm == 0);
    }
    for (int q = -1; q <= 0; q++) {
        COMPROBAR(planificador_ejecutar_v1("rr", &p, 1, q,
            &s, 1, &ns, &m, 1, &nm) == PLANIFICADOR_ARGUMENTO_INVALIDO);
    }
    for (int caso = 0; caso < 9; caso++) {
        ns = nm = -1;
        COMPROBAR(planificador_ejecutar_v1("fcfs", caso == 0 ? NULL : &p,
            caso == 1 ? 0 : caso == 2 ? -1 : 1, 0,
            caso == 3 ? NULL : &s, caso == 4 ? -1 : 1,
            caso == 5 ? NULL : &ns, caso == 6 ? NULL : &m,
            caso == 7 ? -1 : 1, caso == 8 ? NULL : &nm)
            == PLANIFICADOR_ARGUMENTO_INVALIDO);
        COMPROBAR((caso == 5 || ns == 0) && (caso == 8 || nm == 0));
    }
    COMPROBAR(comprobar_segmento(&s, 99, 99, 99, 99));
    COMPROBAR(comprobar_metrica(&m, 98, 98, 98, 98));
    COMPROBAR(planificador_ejecutar_v1("fcfs", &p, 1, -1,
        &s, 1, &ns, &m, 1, &nm) == PLANIFICADOR_OK);
    return 0;
}

static int probar_disposicion_abi_v1(void)
{
    int32_t cantidad = 0;
    const algoritmo_t *catalogo = listar_algoritmos(&cantidad);
    COMPROBAR(catalogo != NULL && cantidad == 4);
    COMPROBAR(listar_algoritmos(NULL) == NULL);
    COMPROBAR(sizeof(algoritmo_t) == 84);
    COMPROBAR(offsetof(algoritmo_t, nombre) == 16);
    COMPROBAR(offsetof(algoritmo_t, requiere_quantum) == 80);
    COMPROBAR(catalogo[0].requiere_quantum == 0 && catalogo[3].requiere_quantum == 1);
    /* Contrato objetivo: Linux x86-64, alineacion natural. */
    COMPROBAR(sizeof(proceso_t) == 32 && sizeof(segmento_t) == 32);
    COMPROBAR(sizeof(metricas_proceso_t) == 32);
    COMPROBAR(offsetof(proceso_t, id) == 0 && offsetof(proceso_t, llegada) == 8);
    COMPROBAR(offsetof(proceso_t, rafaga) == 16 && offsetof(proceso_t, prioridad) == 24);
    COMPROBAR(offsetof(segmento_t, proceso_id) == 0 && offsetof(segmento_t, inicio) == 8);
    COMPROBAR(offsetof(segmento_t, fin) == 16 && offsetof(segmento_t, es_ocioso) == 24);
    COMPROBAR(offsetof(metricas_proceso_t, proceso_id) == 0);
    COMPROBAR(offsetof(metricas_proceso_t, espera) == 8);
    COMPROBAR(offsetof(metricas_proceso_t, retorno) == 16);
    COMPROBAR(offsetof(metricas_proceso_t, respuesta) == 24);
    COMPROBAR(PLANIFICADOR_CANTIDAD_NO_REPRESENTABLE == 5);
    return 0;
}

int main(void)
{
    COMPROBAR(probar_llegadas_desempates_y_ocio() == 0);
    COMPROBAR(probar_un_proceso_y_ocio_inicial() == 0);
    COMPROBAR(probar_validacion_y_capacidad() == 0);
    COMPROBAR(probar_desempates_fcfs() == 0);
    COMPROBAR(probar_errores_no_escriben_resultados() == 0);
    COMPROBAR(probar_exito_v1_equivale_a_fcfs() == 0);
    COMPROBAR(probar_capacidades_insuficientes_v1() == 0);
    COMPROBAR(probar_desbordamiento_v1() == 0);
    COMPROBAR(probar_argumentos_invalidos_v1() == 0);
    COMPROBAR(probar_sjf_llegadas_ocio_no_expropiacion_y_metricas() == 0);
    COMPROBAR(probar_sjf_desempates_provisionales() == 0);
    COMPROBAR(probar_sjf_capacidad_insuficiente_sin_escritura() == 0);
    COMPROBAR(probar_sjf_argumentos_invalidos() == 0);
    COMPROBAR(probar_sjf_desbordamiento_sin_escritura() == 0);
    COMPROBAR(probar_srtf_expropiacion_y_metricas() == 0);
    COMPROBAR(probar_srtf_sin_expropiacion_ni_segmentos_redundantes() == 0);
    COMPROBAR(probar_srtf_llegadas_simultaneas_y_empates() == 0);
    COMPROBAR(probar_srtf_empate_del_proceso_actual() == 0);
    COMPROBAR(probar_srtf_ocio_inicial_e_intermedio() == 0);
    COMPROBAR(probar_srtf_capacidad_insuficiente_sin_escritura() == 0);
    COMPROBAR(probar_srtf_argumentos_invalidos() == 0);
    COMPROBAR(probar_srtf_desbordamiento_sin_escritura() == 0);
    COMPROBAR(probar_rr_basico_y_metricas() == 0);
    COMPROBAR(probar_rr_llegadas_simultaneas_al_vencer_quantum() == 0);
    COMPROBAR(probar_rr_llegada_durante_quantum() == 0);
    COMPROBAR(probar_rr_ocio_y_fusion_de_turnos() == 0);
    COMPROBAR(probar_rr_capacidad_insuficiente_sin_escritura() == 0);
    COMPROBAR(probar_rr_quantum_y_argumentos_invalidos() == 0);
    COMPROBAR(probar_rr_desbordamiento_sin_escritura() == 0);
    COMPROBAR(probar_rr_rafaga_grande_sin_competencia() == 0);
    COMPROBAR(probar_rr_salto_hasta_finalizacion_antes_de_otra_llegada() == 0);
    COMPROBAR(probar_interfaz_comun() == 0);
    COMPROBAR(probar_interfaz_comun_argumentos() == 0);
    COMPROBAR(probar_disposicion_abi_v1() == 0);
    puts("OK: 34 casos nativos FCFS/FIFO, SJF, SRTF, RR e interfaz comun v1");
    return 0;
}
