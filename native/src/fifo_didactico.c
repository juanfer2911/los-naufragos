#include <stdio.h>

#define CANTIDAD_PROCESOS 3
#define QUANTUM 2

// Cada proceso guarda su llegada, su rafaga y el tiempo que le falta.
typedef struct {
    int id;
    int llegada;
    int rafaga;
    int restante;
    int inicio;
    int finalizacion;
} Proceso;

// Los mismos procesos se usan en los cuatro algoritmos para comparar.
void cargarProcesos(Proceso procesos[])
{
    const int llegadas[CANTIDAD_PROCESOS] = {1, 3, 5};
    const int rafagas[CANTIDAD_PROCESOS] = {7, 4, 1};

    for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
        procesos[i].id = i + 1;
        procesos[i].llegada = llegadas[i];
        procesos[i].rafaga = rafagas[i];
        procesos[i].restante = rafagas[i];
        procesos[i].inicio = -1;
        procesos[i].finalizacion = 0;
    }
}

// Busca el proceso pendiente que llegó primero.
int buscarLlegadaMasCercana(Proceso procesos[])
{
    int indiceSiguiente = -1;

    for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
        if (procesos[i].restante > 0
            && (indiceSiguiente == -1
                || procesos[i].llegada < procesos[indiceSiguiente].llegada)) {
            indiceSiguiente = i;
        }
    }

    return indiceSiguiente;
}

// Imprime un tramo de ejecucion o el tiempo ocioso de la CPU.
void imprimirTramo(int idProceso, int inicio, int finalizacion,
                   int *esPrimerTramo)
{
    if (!*esPrimerTramo) {
        printf(" -> ");
    }

    if (idProceso == 0) {
        printf("[%d-%d CPU ociosa]", inicio, finalizacion);
    } else {
        printf("[%d-%d P%d]", inicio, finalizacion, idProceso);
    }

    *esPrimerTramo = 0;
}

// Espera = retorno - rafaga; retorno = finalizacion - llegada.
void imprimirResultados(Proceso procesos[])
{
    printf("\nProceso | Llegada | Rafaga | Inicio | Fin | Espera | Retorno\n");

    for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
        int retorno = procesos[i].finalizacion - procesos[i].llegada;
        int espera = retorno - procesos[i].rafaga;

        printf("P%d      | %7d | %6d | %6d | %3d | %6d | %7d\n",
               procesos[i].id,
               procesos[i].llegada,
               procesos[i].rafaga,
               procesos[i].inicio,
               procesos[i].finalizacion,
               espera,
               retorno);
    }
}

// FCFS: atiende primero al proceso que llegó antes.
void ejecutarFCFS(Proceso procesos[])
{
    int reloj = 0;
    int esPrimerTramo = 1;

    printf("\n=== FCFS / FIFO ===\n");
    printf("Linea de tiempo: ");

    for (int turno = 0; turno < CANTIDAD_PROCESOS; turno++) {
        int indiceSiguiente = buscarLlegadaMasCercana(procesos);

        if (indiceSiguiente == -1) {
            printf("\nError: no se encontro un proceso pendiente.\n");
            return;
        }

        if (procesos[indiceSiguiente].llegada > reloj) {
            imprimirTramo(0, reloj, procesos[indiceSiguiente].llegada,
                          &esPrimerTramo);
            reloj = procesos[indiceSiguiente].llegada;
        }

        // Inicio es el reloj actual; fin es inicio mas la rafaga.
        procesos[indiceSiguiente].inicio = reloj;
        procesos[indiceSiguiente].finalizacion =
            reloj + procesos[indiceSiguiente].rafaga;
        imprimirTramo(procesos[indiceSiguiente].id, reloj,
                      procesos[indiceSiguiente].finalizacion, &esPrimerTramo);

        reloj = procesos[indiceSiguiente].finalizacion;
        procesos[indiceSiguiente].restante = 0;
    }

    imprimirResultados(procesos);
}

// SJF no expulsivo: elige la rafaga mas corta entre los que ya llegaron.
void ejecutarSJF(Proceso procesos[])
{
    int reloj = 0;
    int terminados = 0;
    int esPrimerTramo = 1;

    printf("\n=== SJF no expulsivo ===\n");
    printf("Linea de tiempo: ");

    while (terminados < CANTIDAD_PROCESOS) {
        int indiceSiguiente = -1;

        for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
            if (procesos[i].restante > 0 && procesos[i].llegada <= reloj
                && (indiceSiguiente == -1
                    || procesos[i].rafaga < procesos[indiceSiguiente].rafaga)) {
                indiceSiguiente = i;
            }
        }

        if (indiceSiguiente == -1) {
            int indiceProximaLlegada = buscarLlegadaMasCercana(procesos);

            if (indiceProximaLlegada == -1) {
                printf("\nError: no se encontro un proceso pendiente.\n");
                return;
            }

            // Si nadie ha llegado, la CPU espera hasta la proxima llegada.
            imprimirTramo(0, reloj, procesos[indiceProximaLlegada].llegada,
                          &esPrimerTramo);
            reloj = procesos[indiceProximaLlegada].llegada;
            continue;
        }

        // Una vez elegido, el proceso termina sin ser interrumpido.
        procesos[indiceSiguiente].inicio = reloj;
        procesos[indiceSiguiente].finalizacion =
            reloj + procesos[indiceSiguiente].rafaga;
        imprimirTramo(procesos[indiceSiguiente].id, reloj,
                      procesos[indiceSiguiente].finalizacion, &esPrimerTramo);

        reloj = procesos[indiceSiguiente].finalizacion;
        procesos[indiceSiguiente].restante = 0;
        terminados++;
    }

    imprimirResultados(procesos);
}

// SJF expulsivo (SRTF): cada unidad de tiempo vuelve a elegir la menor rafaga restante.
void ejecutarSRTF(Proceso procesos[])
{
    int reloj = 0;
    int terminados = 0;
    int procesoEnTramo = -1;
    int inicioTramo = 0;
    int esPrimerTramo = 1;

    printf("\n=== SJF expulsivo / SRTF ===\n");
    printf("Linea de tiempo: ");

    while (terminados < CANTIDAD_PROCESOS) {
        int indiceSiguiente = -1;

        for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
            if (procesos[i].restante > 0 && procesos[i].llegada <= reloj
                && (indiceSiguiente == -1
                    || procesos[i].restante
                       < procesos[indiceSiguiente].restante)) {
                indiceSiguiente = i;
            }
        }

        // El identificador 0 representa la CPU ociosa.
        int idSeleccionado = indiceSiguiente == -1
                                 ? 0
                                 : procesos[indiceSiguiente].id;

        // Cuando cambia el elegido, termina el tramo anterior de la linea.
        if (idSeleccionado != procesoEnTramo) {
            if (procesoEnTramo != -1) {
                imprimirTramo(procesoEnTramo, inicioTramo, reloj,
                              &esPrimerTramo);
            }
            procesoEnTramo = idSeleccionado;
            inicioTramo = reloj;
        }

        if (indiceSiguiente == -1) {
            reloj++;
            continue;
        }

        if (procesos[indiceSiguiente].inicio == -1) {
            procesos[indiceSiguiente].inicio = reloj;
        }

        // Avanza una unidad y vuelve a comparar los procesos disponibles.
        procesos[indiceSiguiente].restante--;
        reloj++;

        if (procesos[indiceSiguiente].restante == 0) {
            procesos[indiceSiguiente].finalizacion = reloj;
            terminados++;
        }
    }

    imprimirTramo(procesoEnTramo, inicioTramo, reloj, &esPrimerTramo);
    imprimirResultados(procesos);
}

// Round Robin: cada proceso usa como maximo QUANTUM unidades por turno.
void ejecutarRoundRobin(Proceso procesos[])
{
    int cola[CANTIDAD_PROCESOS];
    int enCola[CANTIDAD_PROCESOS] = {0};
    int frente = 0;
    int final = 0;
    int cantidadEnCola = 0;
    int reloj = 0;
    int terminados = 0;
    int esPrimerTramo = 1;

    printf("\n=== Round Robin (quantum %d) ===\n", QUANTUM);
    printf("Linea de tiempo: ");

    while (terminados < CANTIDAD_PROCESOS) {
        // Los procesos que llegaron se agregan al final de la cola.
        for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
            if (!enCola[i] && procesos[i].restante > 0
                && procesos[i].llegada <= reloj) {
                cola[final] = i;
                final = (final + 1) % CANTIDAD_PROCESOS;
                cantidadEnCola++;
                enCola[i] = 1;
            }
        }

        if (cantidadEnCola == 0) {
            int indiceProximaLlegada = buscarLlegadaMasCercana(procesos);

            if (indiceProximaLlegada == -1) {
                printf("\nError: no se encontro un proceso pendiente.\n");
                return;
            }

            // Si la cola esta vacia, la CPU espera al proximo proceso.
            imprimirTramo(0, reloj, procesos[indiceProximaLlegada].llegada,
                          &esPrimerTramo);
            reloj = procesos[indiceProximaLlegada].llegada;
            continue;
        }

        int indiceActual = cola[frente];
        frente = (frente + 1) % CANTIDAD_PROCESOS;
        cantidadEnCola--;
        enCola[indiceActual] = 0;

        if (procesos[indiceActual].inicio == -1) {
            procesos[indiceActual].inicio = reloj;
        }

        int duracion = QUANTUM;
        if (procesos[indiceActual].restante < QUANTUM) {
            duracion = procesos[indiceActual].restante;
        }

        imprimirTramo(procesos[indiceActual].id, reloj, reloj + duracion,
                      &esPrimerTramo);
        reloj += duracion;
        procesos[indiceActual].restante -= duracion;

        // Los nuevos llegados entran antes de devolver el proceso a la cola.
        for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
            if (i != indiceActual && !enCola[i] && procesos[i].restante > 0
                && procesos[i].llegada <= reloj) {
                cola[final] = i;
                final = (final + 1) % CANTIDAD_PROCESOS;
                cantidadEnCola++;
                enCola[i] = 1;
            }
        }

        if (procesos[indiceActual].restante > 0) {
            // Si no terminó, espera su próximo turno al final de la cola.
            cola[final] = indiceActual;
            final = (final + 1) % CANTIDAD_PROCESOS;
            cantidadEnCola++;
            enCola[indiceActual] = 1;
        } else {
            procesos[indiceActual].finalizacion = reloj;
            terminados++;
        }
    }

    imprimirResultados(procesos);
}

int main(void)
{
    Proceso procesos[CANTIDAD_PROCESOS];

    cargarProcesos(procesos);
    ejecutarFCFS(procesos);

    cargarProcesos(procesos);
    ejecutarSJF(procesos);

    cargarProcesos(procesos);
    ejecutarSRTF(procesos);

    cargarProcesos(procesos);
    ejecutarRoundRobin(procesos);

    return 0;
}
