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

int main(void)
{
    COMPROBAR(probar_llegadas_desempates_y_ocio() == 0);
    COMPROBAR(probar_un_proceso_y_ocio_inicial() == 0);
    COMPROBAR(probar_validacion_y_capacidad() == 0);
    puts("OK: pruebas FCFS/FIFO");
    return 0;
}
