# Los Náufragos — Planificador de procesos PI

Simulador didáctico de CPU: FCFS/FIFO, SJF, SRTF y Round Robin. Presenta segmentos de ejecución y CPU ociosa, y métricas de espera, retorno y respuesta. No ejecuta procesos reales del sistema operativo.

## Estado actual — 2026-11-13

Los cuatro algoritmos están implementados en `native/src/planificador.c`. La API pública está en `native/include/planificador.h`. La interfaz común `planificador_ejecutar_v1()` permite sondear tamaños y ejecutar los cuatro algoritmos mediante identificadores textuales estables: `fcfs`, `sjf`, `srtf`, `rr`. Sustituye al adaptador específico `planificador_fcfs_v1()`, retirado tras verificar su reemplazo y la ausencia de consumidores ejecutables fuera de las pruebas.

La suite contiene 34 casos: conserva las coberturas de los 31 anteriores, con las cuatro pruebas del adaptador adaptadas a la interfaz común. Pruebas y biblioteca verificadas en Ubuntu/WSL 2, sin advertencias de compilación.

`backend/planificador-service/` implementa Java 21/Spring Boot, JNA 5.18.1, catálogo dinámico C, validación JWT de Keycloak y persistencia e historial en PostgreSQL. Round Robin conserva su quantum dentro del snapshot almacenado sin añadirlo a la respuesta pública. Se verificaron 19 pruebas Java y 34 nativas, además de la integración con PostgreSQL y Keycloak reales. Consulta la [guía del microservicio](backend/planificador-service/README.md).

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

La ABI objetivo es Linux x86-64, alineación natural: las tres estructuras de planificación ocupan 32 bytes. El orden y los desplazamientos se comprueban en las pruebas. Java/JNA ejecuta los cuatro algoritmos y consulta el catálogo (estructura de 84 bytes). No se modificaron los desempates existentes; los de SJF/SRTF y el papel de prioridad fuera de FCFS siguen pendientes de confirmación académica.

## Arquitectura acordada

- Angular captura datos y presenta resultados; no ejecuta algoritmos ni calcula promedios.
- Exactamente dos microservicios: `proceso-service` administra conjuntos; `planificador-service` integra C y administra corridas e historial.
- Angular consume directamente ambos servicios, sin API Gateway.
- C mediante JNA y motor Java equivalente de referencia; catálogo dinámico procedente de C, sin enum cerrado en Java/Angular.
- Keycloak/OIDC e historial asociado al claim `sub`; JWT ya validado en `planificador-service` y previsto también para `proceso-service`.
- PostgreSQL 16, Docker Compose local y Docker Swarm como destino de despliegue.

## Organización y documentación

- `native/`: núcleo, cabecera, pruebas y Makefile.
- `native/referencias/`: material didáctico preservado, excluido de compilación.
- `backend/planificador-service/`: REST, seguridad, repositorio JDBC, adaptador JNA y pruebas. Configuración y comandos en su README.
- `infra/`: Compose de PostgreSQL/Keycloak verificado; cada componente usa su base: `planificador_db`, `proceso_db` (servicio pendiente) y `keycloak`.
- `frontend/`: reservado al trabajo de Angular de otra integrante; integración pendiente. `tests/`: suites independientes de benchmark aún pendientes.
- [Resumen de continuidad](docs/resumen-contexto.md), [bitácora](docs/bitacora-decisiones.md) y [arquitectura](docs/arquitectura-add.md).
- [OpenAPI](docs/openapi.yaml): contrato compartido conservado sin modificaciones; cuatro operaciones implementadas. `/comparaciones` y las operaciones de conjuntos siguen pendientes. Mantiene identificadores textuales y catálogo abierto.

Los PDF, DOCX y wireframes originales en `docs/` se conservan como fuentes académicas.

Falta desarrollar `proceso-service`, integrar Angular y preparar el despliegue final en Swarm. También quedan el motor Java de referencia, `/comparaciones`, benchmarks y validaciones académicas pendientes descritas en el resumen.
