# Los Náufragos — Planificador de procesos PI

Simulador didáctico de CPU: FCFS/FIFO, SJF, SRTF y Round Robin. Presenta segmentos de ejecución y CPU ociosa, y métricas de espera, retorno y respuesta. No ejecuta procesos reales del sistema operativo.

## Estado actual - 2026-11-20

Los cuatro algoritmos están implementados en `native/src/planificador.c`. La API pública está en `native/include/planificador.h`. La interfaz común `planificador_ejecutar_v1()` permite sondear tamaños y ejecutar los cuatro algoritmos mediante identificadores textuales estables: `fcfs`, `sjf`, `srtf`, `rr`. Sustituye al adaptador específico `planificador_fcfs_v1()`, retirado tras verificar su reemplazo y la ausencia de consumidores ejecutables fuera de las pruebas.

La suite contiene **42 grupos nativos** para los cuatro algoritmos y la interfaz común v1. La verificación más reciente en Ubuntu/WSL 2 compiló y pasó los 42 grupos, sin advertencias.

`backend/planificador-service/` implementa Java 21/Spring Boot, JNA 5.18.1, catálogo dinámico C, validación JWT de Keycloak y persistencia e historial en PostgreSQL. Round Robin conserva su quantum dentro del snapshot almacenado sin añadirlo a la respuesta pública. Hay 25 pruebas Java registradas para el servicio y la integración C/JNA; las pruebas nativas más recientes pasaron 42 grupos. La integración con PostgreSQL y Keycloak reales también está documentada. Consulta la [guía del microservicio](backend/planificador-service/README.md).

El microservicio incluye un motor Java de referencia seleccionable con `PLANIFICADOR_MOTOR=java`; el valor predeterminado sigue siendo `nativo`. Los dos bancos independientes C/JNA frente a Java están en `tests/benchmarks/`: Web 2 (cuatro tamaños, 30 repeticiones) y Arquitectura (SRTF, 10.000 procesos, cinco corridas y descarte de las primeras tres). Las ejecuciones locales del 2026-11-16 produjeron 960 comparaciones Web 2 y 10 de Arquitectura, todas idénticas. Las mediciones WSL sin límites están separadas de las ejecuciones Swarm limitadas; protocolos y resultados de ambos perfiles están en esa carpeta.

Los endpoints protegidos son `GET /algoritmos`, `POST /planificaciones` (201), `GET /historial` y `GET /historial/{planificacionId}`. Cada historial pertenece al `sub` del JWT validado. El servidor local usa `http://localhost:8081`, Java Linux/WSL y la biblioteca compilada en `native/`. Para iniciarlo, seguir la [guía Docker Desktop/WSL](backend/planificador-service/docs/verificacion-real.md).

## Compilar y probar el núcleo

Desde la raíz del repositorio, en Ubuntu/WSL, con GCC y Make:

```sh
make -C native clean
make -C native test
make -C native library
nm -D --defined-only native/libplanificador.so
```

`clean` elimina únicamente el ejecutable de pruebas y la biblioteca generados por el Makefile. Se utiliza C11 con `-Wall -Wextra -Werror -pedantic`. La biblioteca resultante es Linux; la JVM debe ejecutarse en una plataforma y arquitectura compatibles.

## Contrato nativo v1

`planificador_ejecutar_v1()` recibe el identificador ASCII sensible a mayúsculas, procesos, cantidad, quantum y buffers con capacidades y contadores `int32_t`; los tiempos y el quantum son `int64_t`. Solo RR exige quantum positivo; los otros algoritmos lo ignoran.

Para consultar tamaños, pasar ambos buffers de salida NULL con capacidades cero y contadores válidos y distintos. Devuelve estado 3 y cantidades requeridas exactas. Reservar buffers y repetir con la misma entrada. Las métricas mantienen el orden de los procesos y los segmentos incluyen ociosidad. Los buffers deben ser válidos, suficientes y no solaparse; la entrada debe permanecer estable durante las llamadas.

Estados: 0 éxito; 1 argumento inválido (incluido identificador desconocido); 2 memoria insuficiente; 3 capacidad insuficiente, sin resultados parciales; 4 desbordamiento de tiempo; 5 segmentos requeridos no representables en `int32_t`. En error previo al sondeo, contadores cero; para estado 5, segmentos cero y métricas igual a cantidad de procesos; un error posterior conserva los tamaños sondeados.

La ABI objetivo es Linux x86-64, alineación natural: las tres estructuras de planificación ocupan 32 bytes. El orden y los desplazamientos se comprueban en las pruebas. Java/JNA ejecuta los cuatro algoritmos y consulta el catálogo (estructura de 84 bytes). Las reglas de desempate están documentadas en `native/README.md` y aprobadas por el equipo; la aprobación del profesor no se presume.

## Arquitectura acordada

- Angular captura datos y presenta resultados; no ejecuta algoritmos ni calcula promedios.
- Exactamente dos microservicios: `proceso-service` administra conjuntos; `planificador-service` integra C y administra corridas e historial.
- Angular consume directamente ambos servicios, sin API Gateway.
- C mediante JNA y motor Java equivalente seleccionable; el modo nativo usa el catálogo dinámico de C.
- Keycloak/OIDC e historial asociado al claim `sub`; JWT ya validado en `planificador-service` y previsto también para `proceso-service`.
- PostgreSQL 16, Docker Compose local y Docker Swarm como destino de despliegue.

## Organización y documentación

- `native/`: núcleo, cabecera, pruebas y Makefile.
- `native/referencias/`: material didáctico preservado, excluido de compilación.
- `backend/planificador-service/`: REST, seguridad, repositorio JDBC, adaptador JNA y pruebas. Configuración y comandos en su README.
- `infra/`: Compose local de PostgreSQL/Keycloak; cada componente usa su base: `planificador_db`, `proceso_db` y `keycloak`.
- `frontend/`: captura y presenta resultados; tabla comparativa conjunta implementada. `tests/benchmarks/`: suites y resultados crudos/medianas de los dos protocolos.
- [Resumen de continuidad](docs/resumen-contexto.md), [bitácora](docs/bitacora-decisiones.md) y [arquitectura](docs/arquitectura-add.md).
- [OpenAPI](docs/openapi.yaml): contrato compartido con identificadores textuales y catálogo abierto. Están implementadas las operaciones de planificación, historial y conjuntos; `/comparaciones` está propuesto en OpenAPI, pero Angular compara ejecutando las operaciones de planificación existentes.

Los PDF, DOCX y wireframes originales en `docs/` se conservan como fuentes académicas.


## Estado de cierre y pendientes

Al 2026-11-19 se ejecutaron en la interfaz los seis controles del equipo; los cuatro algoritmos coincidieron con Gantt, metricas, promedios y tiempos. Se registraron HTTP 403 para POST/DELETE estudiante, HTTP 401 para JWT alterado en ambos servicios y una renovacion OIDC observada. Evidencias en [controles manuales](docs/controles-manuales.md) y [seguridad/usabilidad](docs/evidencias-seguridad-usabilidad.md). Consulta el [anexos de entrega](docs/anexos-entrega.md) para la lista de evidencia y lo que sigue pendiente.

Las cinco imagenes runtime fueron medidas y el redeploy con el volumen PostgreSQL existente estuvo listo en 97,718 s. Datos crudos y limites en [benchmarks](tests/benchmarks/README.md). El stack Swarm esta activo.

Pendiente de personas externas: tres sesiones RQ-03. Pendiente academico: cotejar controles oficiales si se entregan, completar anexos/sustentacion y reservar el quinto algoritmo para la demostracion en vivo. El plan academico indica no implementarlo anticipadamente.
