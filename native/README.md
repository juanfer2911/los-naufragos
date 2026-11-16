# Núcleo de planificación

Reglas definitivas aprobadas por el equipo el 2026-11-16 para la etapa 1. Esta aprobación no se atribuye al profesor: los PDF exigen políticas documentadas y deterministas, pero no enumeran todos estos desempates. No se implementa un quinto algoritmo ni se preparan aquí los seis controles académicos de las etapas siguientes.

## Convenciones

- Solo participan procesos cuya llegada sea menor o igual al reloj actual.
- Menor número de prioridad significa mayor prioridad, sin desplazar el criterio principal de cada algoritmo.
- El PID identifica; no ordena candidatos. El último desempate es el índice original de la lista, que no equivale al PID.
- Las métricas se devuelven en orden de entrada. En C, `es_ocioso` marca explícitamente el ocio; un proceso real puede tener PID 0.
- Los intervalos del Gantt son `[inicio, fin)` y representan ocupación continua de CPU. SRTF y RR fusionan segmentos contiguos del mismo proceso. Una llegada sin cambio de proceso no crea otra barra SRTF; varios turnos RR consecutivos pueden verse como una sola barra mayor que el quantum.
- Cada turno lógico RR dura como máximo quantum o hasta finalizar. Sin competencia antes de terminar, la implementación salta turnos redundantes sin cambiar la planificación observable. Esta optimización se conserva.

## Selección y desempates

Los criterios se aplican de izquierda a derecha. El ingreso es la posición original, incluso con PID desordenados.

| Algoritmo | Selección | Interrupción |
|---|---|---|
| FCFS | Llegada → prioridad → ingreso | No expulsivo: termina toda su ráfaga |
| SJF | Entre disponibles: ráfaga → prioridad → llegada → ingreso | No expulsivo: una llegada más corta o prioritaria no lo interrumpe |
| SRTF | Menor restante; el activo conserva CPU si empata, aunque tenga peor prioridad. Sin activo que deba conservarse: restante → prioridad → llegada → ingreso | Solo por un restante estrictamente menor; prioridad por sí sola nunca desaloja |
| RR | Cola FIFO circular; llegadas por llegada → prioridad → ingreso | Por quantum o finalización, no por prioridad |

FCFS continúa en `comparar_procesos_fcfs`. SJF/SRTF cambian únicamente el orden de `es_preferible_sjf`/`es_preferible_srtf`; se conserva la protección del activo en `seleccionar_proceso_srtf`. RR añade prioridad a su estructura **interna** `llegada_rr_t` y a `comparar_llegadas_rr`. No cambian estructuras públicas, firmas, catálogo, JNA ni JSON.

## Cola RR y equivalencia con FCFS

Los que ya esperan conservan su posición. Al terminar un turno se añaden al final las llegadas pendientes, incluidas las que llegan exactamente al final del quantum, ordenadas cronológicamente; solo dentro del mismo instante se ordenan por prioridad e ingreso. Después se reinserta al activo si le queda ráfaga. Si terminó, no vuelve a la cola. Nunca se ordena toda la cola por prioridad.

Con quantum mayor que todas las ráfagas, cada proceso termina en su primer turno y no hay reinserciones. FCFS y RR comparten ahora el orden llegada/prioridad/ingreso: producen la misma secuencia y métricas incluso con llegadas simultáneas. No se añade una excepción que llame a FCFS según el quantum.

## Regresiones con esperados independientes

Los esperados son literales calculados a mano. Ninguna prueba genera su oráculo ejecutando otro algoritmo. Se aplicó retorno = fin − llegada, espera = retorno − ráfaga y respuesta = primer inicio − llegada, comparando **todos** los segmentos y métricas.

Caso obligatorio SRTF: P1 `(llegada=0, ráfaga=4, prioridad=3)` y P2 `(2,2,1)`. En t=2 ambos tienen restante 2 y P1 conserva CPU:

```text
P1 [0,4) → P2 [4,6)
P1: espera 0, retorno 4, respuesta 0
P2: espera 2, retorno 4, respuesta 2
```

Regresión RR, quantum 2, orden de entrada: P90 `(0,4,1)`, P7 `(0,1,3)`, P20 `(1,1,9)`, P2 `(2,1,1)` y P3 `(2,1,0)`. En t=2 la cola es `[P7, P20, P3, P2, P90]`: P7 ya esperaba; P20 llegó antes; P3 precede a P2 por prioridad; P90 se reinserta al final.

```text
P90 [0,2) → P7 [2,3) → P20 [3,4) → P3 [4,5) → P2 [5,6) → P90 [6,8)
```

Pruebas nuevas en `tests/test_planificador.c`:

| Grupo | Qué demuestra |
|---|---|
| `probar_fcfs_y_rr_quantum_grande_reglas` | Prioridad en llegadas iguales, ingreso distinto de PID, llegada antes de prioridad y equivalencia RR/FCFS, ambos contra el mismo esperado manual |
| `probar_sjf_reglas_aprobadas` | No desaloja al activo ante procesos más cortos; candidatos por prioridad, llegada e ingreso |
| `probar_srtf_empate_obligatorio` | P1/P2 solicitado y sus métricas completas |
| `probar_srtf_desalojo_estricto_y_ocio` | Restante menor desaloja aunque el nuevo tenga peor prioridad; ocio y respuesta correctos |
| `probar_srtf_candidatos_esperando` | Los tres desempates entre candidatos esperando |
| `probar_rr_llegadas_no_reordenan_cola` | Conserva los que esperan, orden de llegadas y reinserción posterior |
| `probar_rr_prioridad_simultanea_y_fusion` | Prioridades del grupo inicial, empate por ingreso y fusión |
| `probar_reglas_ocio_pid_cero` | Los cuatro algoritmos distinguen ocio de un proceso PID 0 y conservan sus métricas |

La prueba SJF anterior cambia su orden esperado de `1,3,4,2,5` a `1,5,3,4,2`: P5 tiene la misma ráfaga y mayor prioridad, aunque llegó después. Se mantienen las otras regresiones. `PlanificadorNativoTests.java` también contrasta estas reglas con la biblioteca compartida real vía JNA.

## Ejecutar

Desde la raíz, en Ubuntu/WSL con GCC/Make:

```sh
make -C native test
make -C native library
```

`make test` compila con C11, `-Wall -Wextra -Werror -pedantic -O2`. No sobrescribir un `.so` que una JVM tenga cargado: detener únicamente esa JVM antes de sustituirlo o compilar una copia separada para pruebas. La nueva biblioteca entra en uso en una JVM nueva; no se recalculan históricos guardados.

La única cabecera oficial del núcleo es `include/planificador.h` (`-Iinclude` en Makefile). Se eliminó `native/planificador.h`, una cabecera histórica con otra interfaz y límites fijos: la revisión de referencias y las dependencias emitidas por el compilador (`cc -Iinclude -MM src/planificador.c tests/test_planificador.c`) demostraron que no tenía consumidores activos. La interfaz ilustrativa sigue disponible en los PDF originales; no se modifica la ABI actual. También quedan sin cambios el sondeo/ejecución de la ABI y los promedios en Spring: esta aprobación cierra reglas de planificación, no los restantes pendientes académicos.

## Verificación de esta etapa

El 2026-11-16 pasaron 42 grupos C (34 anteriores y ocho nuevos), 25 pruebas Java del planificador con C/JNA real (cero fallos, errores u omisiones), 14 pruebas Angular y el build Angular. Permanecen los avisos conocidos de presupuesto CSS y CommonJS de Keycloak. Se probó primero un `.so` separado bajo `backend/planificador-service/target/` para no sobrescribir la biblioteca cargada; después se detuvo únicamente la JVM del planificador, se construyó `native/libplanificador.so` y se inició de nuevo con la configuración existente. No se cambiaron cuentas, contraseñas ni datos existentes. No se ejecutaron benchmarks ni se añadieron controles de la etapa 2. La consolidación posterior volvió a ejecutar estas suites y las siete pruebas de proceso-service tras retirar la cabecera antigua; el resultado de publicación se informa al cierre de esa tarea.
