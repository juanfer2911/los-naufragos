# Controles manuales de planificaciÃ³n

**VersiÃ³n:** 2026-11-18. Controles preparados por el equipo para verificar la interfaz y comparar sus resultados con cÃ¡lculos independientes.

> Los PDF acadÃ©micos citados en el repositorio no estÃ¡n presentes en este checkout. Por eso estos seis casos cubren las categorÃ­as registradas en `resumen-contexto.md`, pero no se atribuyen al profesor ni sustituyen los datos oficiales si el curso los proporciona.

## CÃ³mo ejecutarlos

Para cada caso, cargar exactamente los procesos indicados, activar los cuatro algoritmos y usar el quantum indicado para Round Robin. Comparar segmentos, mÃ©tricas por proceso y promedios con estas tablas. Los intervalos son semiabiertos `[inicio, fin)`; `ocio` es CPU ociosa. Las mÃ©tricas por proceso aparecen como `espera/retorno/respuesta`, en milisegundos. Los promedios se redondean a dos decimales.

FÃ³rmulas independientes: `retorno = fin - llegada`; `espera = retorno - rÃ¡faga`; `respuesta = primer inicio - llegada`.

Los casos tambiÃ©n tienen aserciones C/JNA en `backend/planificador-service/src/test/java/com/losnaufragos/planificador/nativo/PlanificadorNativoTests.java`. Los builds Docker del 2026-11-18 ejecutaron esas pruebas junto con las suites HTTP; los promedios aquÃ­ se obtienen aplicando las fÃ³rmulas anteriores a cada fila por proceso.

## C1 â€” Llegadas simultÃ¡neas y prioridades repetidas

Entrada en orden: `PID (llegada, rÃ¡faga, prioridad)`: `2 (0,4,2)`, `1 (0,2,1)`, `3 (0,1,1)`. Quantum RR: `1`.

| Algoritmo | Gantt esperado | MÃ©tricas por PID: espera/retorno/respuesta | Promedios: espera/retorno/respuesta |
|---|---|---|---|
| FIFO | P1 [0,2) â†’ P3 [2,3) â†’ P2 [3,7) | P2 3/7/3; P1 0/2/0; P3 2/3/2 | 1,67 / 4,00 / 1,67 |
| SJF | P3 [0,1) â†’ P1 [1,3) â†’ P2 [3,7) | P2 3/7/3; P1 1/3/1; P3 0/1/0 | 1,33 / 3,67 / 1,33 |
| SRTF | P3 [0,1) â†’ P1 [1,3) â†’ P2 [3,7) | P2 3/7/3; P1 1/3/1; P3 0/1/0 | 1,33 / 3,67 / 1,33 |
| RR | P1 [0,1) â†’ P3 [1,2) â†’ P2 [2,3) â†’ P1 [3,4) â†’ P2 [4,7) | P2 3/7/2; P1 2/4/0; P3 1/2/1 | 2,00 / 4,33 / 1,00 |

## C2 â€” RÃ¡fagas iguales y orden de ingreso distinto del PID

Entrada en orden: `8 (0,2,1)`, `3 (0,2,1)`. Quantum RR: `1`.

| Algoritmo | Gantt esperado | MÃ©tricas por PID: espera/retorno/respuesta | Promedios: espera/retorno/respuesta |
|---|---|---|---|
| FIFO | P8 [0,2) â†’ P3 [2,4) | P8 0/2/0; P3 2/4/2 | 1,00 / 3,00 / 1,00 |
| SJF | P8 [0,2) â†’ P3 [2,4) | P8 0/2/0; P3 2/4/2 | 1,00 / 3,00 / 1,00 |
| SRTF | P8 [0,2) â†’ P3 [2,4) | P8 0/2/0; P3 2/4/2 | 1,00 / 3,00 / 1,00 |
| RR | P8 [0,1) â†’ P3 [1,2) â†’ P8 [2,3) â†’ P3 [3,4) | P8 1/3/0; P3 2/4/1 | 1,50 / 3,50 / 0,50 |

## C3 â€” Ocio inicial e intermedio

Entrada en orden: `5 (2,3,1)`, `6 (8,2,1)`. Quantum RR: `2`.

| Algoritmo | Gantt esperado | MÃ©tricas por PID: espera/retorno/respuesta | Promedios: espera/retorno/respuesta |
|---|---|---|---|
| FIFO | ocio [0,2) â†’ P5 [2,5) â†’ ocio [5,8) â†’ P6 [8,10) | P5 0/3/0; P6 0/2/0 | 0,00 / 2,50 / 0,00 |
| SJF | ocio [0,2) â†’ P5 [2,5) â†’ ocio [5,8) â†’ P6 [8,10) | P5 0/3/0; P6 0/2/0 | 0,00 / 2,50 / 0,00 |
| SRTF | ocio [0,2) â†’ P5 [2,5) â†’ ocio [5,8) â†’ P6 [8,10) | P5 0/3/0; P6 0/2/0 | 0,00 / 2,50 / 0,00 |
| RR | ocio [0,2) â†’ P5 [2,5) â†’ ocio [5,8) â†’ P6 [8,10) | P5 0/3/0; P6 0/2/0 | 0,00 / 2,50 / 0,00 |

## C4 â€” Quantum RR mayor que toda rÃ¡faga

Entrada en orden: `1 (0,4,2)`, `2 (1,2,1)`, `3 (2,1,1)`. Quantum RR: `10` (mÃ¡xima rÃ¡faga: `4`).

| Algoritmo | Gantt esperado | MÃ©tricas por PID: espera/retorno/respuesta | Promedios: espera/retorno/respuesta |
|---|---|---|---|
| FIFO | P1 [0,4) â†’ P2 [4,6) â†’ P3 [6,7) | P1 0/4/0; P2 3/5/3; P3 4/5/4 | 2,33 / 4,67 / 2,33 |
| SJF | P1 [0,4) â†’ P3 [4,5) â†’ P2 [5,7) | P1 0/4/0; P2 4/6/4; P3 2/3/2 | 2,00 / 4,33 / 2,00 |
| SRTF | P1 [0,1) â†’ P2 [1,3) â†’ P3 [3,4) â†’ P1 [4,7) | P1 3/7/0; P2 0/2/0; P3 1/2/1 | 1,33 / 3,67 / 0,33 |
| RR | P1 [0,4) â†’ P2 [4,6) â†’ P3 [6,7) | P1 0/4/0; P2 3/5/3; P3 4/5/4 | 2,33 / 4,67 / 2,33 |

## C5 â€” Un proceso y ocio inicial

Entrada: `9 (4,5,2)`. Quantum RR: `2`.

| Algoritmo | Gantt esperado | MÃ©tricas por PID: espera/retorno/respuesta | Promedios: espera/retorno/respuesta |
|---|---|---|---|
| FIFO | ocio [0,4) â†’ P9 [4,9) | P9 0/5/0 | 0,00 / 5,00 / 0,00 |
| SJF | ocio [0,4) â†’ P9 [4,9) | P9 0/5/0 | 0,00 / 5,00 / 0,00 |
| SRTF | ocio [0,4) â†’ P9 [4,9) | P9 0/5/0 | 0,00 / 5,00 / 0,00 |
| RR | ocio [0,4) â†’ P9 [4,9) | P9 0/5/0 | 0,00 / 5,00 / 0,00 |

## C6 â€” Prioridades repetidas con llegadas escalonadas

Entrada en orden: `1 (0,4,2)`, `2 (1,2,2)`, `3 (3,1,2)`. Quantum RR: `2`.

| Algoritmo | Gantt esperado | MÃ©tricas por PID: espera/retorno/respuesta | Promedios: espera/retorno/respuesta |
|---|---|---|---|
| FIFO | P1 [0,4) â†’ P2 [4,6) â†’ P3 [6,7) | P1 0/4/0; P2 3/5/3; P3 3/4/3 | 2,00 / 4,33 / 2,00 |
| SJF | P1 [0,4) â†’ P3 [4,5) â†’ P2 [5,7) | P1 0/4/0; P2 4/6/4; P3 1/2/1 | 1,67 / 4,00 / 1,67 |
| SRTF | P1 [0,1) â†’ P2 [1,3) â†’ P3 [3,4) â†’ P1 [4,7) | P1 3/7/0; P2 0/2/0; P3 0/1/0 | 1,00 / 3,33 / 0,00 |
| RR | P1 [0,2) â†’ P2 [2,4) â†’ P1 [4,6) â†’ P3 [6,7) | P1 2/6/0; P2 1/3/1; P3 3/4/3 | 2,00 / 4,33 / 1,33 |

## Registro de ejecuciÃ³n

La ejecución en la interfaz y su alcance se registran abajo; no equivalen a aceptación del docente. Registrar cada ejecuciÃ³n y adjuntar la captura del caso de seis procesos si el docente entrega ese control oficial. Las tres sesiones de usabilidad se registran por separado en [evidencias-seguridad-usabilidad.md](evidencias-seguridad-usabilidad.md).

### EjecuciÃ³n en la interfaz â€” 2026-11-19

Los seis casos se recorrieron en la aplicaciÃ³n desplegada localmente (`http://localhost:4200`) con Chrome automatizado mediante Playwright y una cuenta temporal de estudiante de Keycloak, eliminada al terminar. Para cada caso se ejecutaron FIFO, SJF, SRTF y RR; se cotejaron segmentos Gantt, mÃ©tricas por proceso, promedios y marcas de tiempo con esta matriz. TambiÃ©n se comprobÃ³ la tabla comparativa visible.

| Caso | Resultado | ComprobaciÃ³n |
|---|---|---|
| C1 | Aprobado | Cuatro algoritmos; Gantt y mÃ©tricas coinciden |
| C2 | Aprobado | Cuatro algoritmos; Gantt y mÃ©tricas coinciden |
| C3 | Aprobado | Cuatro algoritmos; Gantt, ocio y mÃ©tricas coinciden |
| C4 | Aprobado | Cuatro algoritmos; Gantt y mÃ©tricas coinciden |
| C5 | Aprobado | Cuatro algoritmos; ocio y mÃ©tricas coinciden |
| C6 | Aprobado | Cuatro algoritmos; Gantt y mÃ©tricas coinciden |

**Alcance del cotejo:** la interfaz asigna los PID automÃ¡ticamente en orden ascendente y no permite escribir los PID incluidos en la matriz. Se ingresaron los mismos procesos en el mismo orden y se relacionaron sus PID por posiciÃ³n para comparar los resultados. Por eso se verificaron las polÃ­ticas, los segmentos y las mÃ©tricas, pero no se probÃ³ en la interfaz la numeraciÃ³n literal de PID no consecutivos o ingresados fuera de orden. Los registros de planificaciÃ³n generados durante la ejecuciÃ³n se eliminaron de PostgreSQL al finalizar.
