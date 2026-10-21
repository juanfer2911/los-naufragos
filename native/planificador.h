#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#define MAX_PROCESOS 100
#define MAX_REBANADAS 500

// Estructura de entrada para un proceso
typedef struct {
    int pid;
    int llegada;
    int rafaga;
    int prioridad;
} Proceso;

// Segmento del diagrama de Gantt (pid = -1 representa CPU ociosa)
typedef struct {
    int pid;
    int inicio;
    int fin;
} Rebanada;

// Métricas resultantes por proceso
typedef struct {
    int pid;
    int espera;
    int retorno;
    int respuesta;
} Metrica;

// Estructura completa devuelta por la simulación
typedef struct {
    Rebanada rebanadas[MAX_REBANADAS];
    int n_rebanadas;
    Metrica metricas[MAX_PROCESOS];
    int n_metricas;
    double espera_prom;
    double retorno_prom;
    double respuesta_prom;
} Resultado;

// Funciones expuestas en la librería dinámica libplanificador.so
int planificar(const char *algoritmo, const Proceso *procs, int n, int quantum, Resultado *salida);
int listar_algoritmos(char *buffer, int tam);

#endif