# Resumen de contexto y continuidad

**Proyecto:** Los Náufragos — Planificador de procesos PI  
**Última revisión del estado real:** 2026-10-22
**Propósito de este archivo:** resumir el estado verificable para que el equipo y un agente posterior puedan continuar sin confundir decisiones, propuestas documentales e implementación.

Este documento vivo no sustituye la bitácora. `docs/bitacora-decisiones.md` conserva las decisiones y sus razones; este resumen describe el estado actual observado, las tareas pendientes y la siguiente acción recomendada. Si ambos documentos difieren, verificar con el equipo y no reescribir silenciosamente el historial.

## 1. Identificación y objetivo del proyecto

El Planificador de procesos PI es un simulador didáctico de planificación de CPU del equipo **Los Náufragos**. Recibe conjuntos de procesos, permite ejecutar y comparar algoritmos, y presenta diagramas de Gantt con segmentos de ejecución y CPU ociosa, además de métricas de espera, retorno y respuesta. No ejecuta procesos reales del sistema operativo.

Las fuentes principales son el enunciado general de Web 2, el documento específico del equipo, el documento del núcleo nativo y la guía maestra.

## 2. Arquitectura y decisiones confirmadas

Estas son decisiones confirmadas por el equipo y registradas en la bitácora actual:

- **Frontend:** Angular con componentes standalone y signals. La interfaz captura datos, consume servicios y representa resultados; no ejecuta algoritmos ni calcula promedios.
- **Dos microservicios, exactamente:** `proceso-service` administra procesos y conjuntos persistidos; `planificador-service` ejecuta planificaciones, integra el motor nativo y gestiona historial. El profesor confirmó que el proyecto se realizará con dos microservicios. **No añadir un tercero.**
- **Sin API Gateway.** Angular se comunica directamente con los endpoints de ambos servicios.
- **Núcleo nativo:** algoritmos en C, compilados como `libplanificador.so` e invocados desde Java mediante JNA.
- **Motor Java:** se requiere una implementación de referencia equivalente, seleccionable mediante configuración para comparación y respaldo.
- **Identidad:** Keycloak proporciona OIDC/OAuth 2.0. Ambos microservicios validarán JWT y aplicarán autorización en backend.
- **Persistencia:** PostgreSQL. `proceso-service` posee los datos de sus conjuntos; el historial pertenece a `planificador-service`.
- **Propiedad de corridas:** cada registro de historial se asocia al claim `sub` del JWT. El usuario consulta únicamente su historial; Angular no determina libremente el propietario.
- **Origen de una simulación:** Angular envía la lista completa de procesos en el cuerpo de `POST /planificaciones`; no hace falta que la carga esté persistida en `proceso-service`.
- **Algoritmos base:** FCFS/FIFO, SJF no expropiativo, SRTF expropiativo y Round Robin con quantum configurable.
- **Catálogo dinámico:** `listar_algoritmos()` de la biblioteca C alimenta `GET /algoritmos`. No se deben fijar listas cerradas en Angular o en la lógica de negocio Java. Añadir un algoritmo en C es un requisito de diseño aún no implementado ni comprobado.
- **Prioridad:** es un atributo numérico, no un algoritmo adicional. Menor número significa mayor prioridad. La regla general confirmada indica prioridad, llegada y orden original. Para **FCFS/FIFO**, el equipo precisó el 2026-10-21 que prevalece la llegada; solo entre procesos con igual llegada se usa menor número de prioridad y, si también empatan, orden original. Esto conserva FCFS estricto; la aplicación concreta de la regla en SJF, SRTF y Round Robin sigue pendiente.
- **Entorno:** Docker Compose es para integración local; Docker Swarm es el objetivo de despliegue. No crear manifiestos de Kubernetes.
- **Benchmarks:** son suites independientes con scripts separados en `tests/`:
  - Web 2: cargas de 50, 500, 5.000 y 50.000 procesos, 30 repeticiones por carga y mediana del tiempo de respuesta.
  - Arquitectura de Computadores: SRTF con 10.000 procesos, cinco ejecuciones consecutivas y descarte de las primeras tres por calentamiento JIT. Quedan dos ejecuciones posteriores al descarte, según la decisión confirmada; no mezclar las metodologías.

La decisión de dos servicios no se considera pendiente aunque el enunciado general mencione un mínimo de tres. La bitácora registra que el profesor confirmó los dos para este proyecto.

## 3. Estructura real del repositorio

Árbol relevante observado. Las carpetas que aparecen vacías son estructura existente, no componentes implementados.

```text
.
├── backend/
│   └── planificador-service/        # Esqueleto Maven/Spring Boot; sin endpoints ni JNA
├── docs/
│   ├── - Los Naufragos - Planificador de procesos.pdf
│   ├── Enunciado general_PI.pdf
│   ├── Guia_maestra_Copilot_Planificador_Los_Naufragos.docx
│   ├── Los Naufragos - Nucleo de planificacion.pdf
│   ├── arquitectura-add.md
│   ├── bitacora-decisiones.md
│   ├── openapi.yaml
│   ├── resumen-contexto.md          # Este documento
│   └── wireframes/
├── frontend/                        # Vacía; no hay aplicación Angular
├── infra/
│   ├── .env.example
│   ├── README.md
│   ├── docker-compose.yml
│   └── postgres/init/01-create-databases.sql
├── native/
│   ├── include/planificador.h        # API nativa para la implementación y prueba local
│   ├── src/planificador.c            # Implementación FCFS/FIFO
│   ├── tests/test_planificador.c     # Pruebas FCFS/FIFO y ABI v1
│   ├── libplanificador.so           # Biblioteca Linux x86-64 presente al revisar
│   └── Makefile                     # Pruebas y objetivo separado library, C11
├── tests/                           # Vacía; no hay suites ni scripts
├── .gitignore
└── README.md
```

`backend/planificador-service/` contiene ahora únicamente la estructura inicial Maven/Spring Boot descrita en “Estado actual y siguiente paso”; `backend/proceso-service/`, los endpoints y la integración JNA todavía no existen. `frontend/` y la carpeta superior `tests/` siguen sin archivos. Los resultados anteriores de pruebas y generación de la biblioteca nativa se detallan en la sección de estado actual.

Git está inicializado en la rama `feature/juanfer`, que sigue a `origin/feature/juanfer`. Antes de crear el microservicio, el resumen ya estaba modificado y los cuatro archivos nativos tenían cambios locales; no se había creado ningún commit.

## 4. Tecnologías y versiones

| Tecnología | Estado según archivos revisados |
|---|---|
| Angular 17 o superior, standalone y signals | Rango acordado y documentado en la bitácora. No hay `package.json`, fuentes ni proyecto Angular; versión instalada/configurada **no verificada**. |
| Java 21 LTS | Configurado en el POM; la prueba Maven pasó usando Java 21.0.12.1. |
| Spring Boot 3.2 o superior compatible con Java 21 | `planificador-service` usa Spring Boot 3.5.7; la prueba de contexto pasó. |
| C con GCC, estándar C11 o C17 | `native/Makefile` solicita C11. FCFS/FIFO, ABI v1 y la biblioteca compartida están en el repositorio; pruebas y compilaciones exitosas fueron reportadas anteriormente y deben repetirse si hace falta confirmarlas. |
| JNA 5.x | Rango acordado; no hay dependencia Java configurada ni interfaz JNA. Versión exacta **pendiente**. |
| PostgreSQL 16 | Versión mayor acordada. `infra/docker-compose.yml` fija `postgres:16.4`; el archivo pasa la validación estática de Compose, pero contenedor y base **no verificados en ejecución**. |
| Keycloak 24 o superior | Mínimo acordado. Compose fija `quay.io/keycloak/keycloak:25.0.6`, conectado a PostgreSQL. No se comprobó que el contenedor iniciara ni existe importación del realm de aplicación. |
| Docker Compose / Docker Swarm | Compose está definido para la infraestructura local base; Swarm está confirmado como objetivo futuro. No hay stack Swarm ni despliegue completo. |

Las versiones acordadas en la bitácora no implican dependencias instaladas ni proyectos compilables. Las etiquetas de imágenes son configuración declarada, no evidencia de que el entorno se haya desplegado correctamente.

## 5. Estado actual del proyecto

Estados utilizados en la tabla: **Completado**, **En progreso**, **Pendiente** y **Por verificar**. “Completado” se aplica solo a lo comprobado en archivos o pruebas, no por la mera existencia de un diseño.

| Área o tarea | Estado | Evidencia | Siguiente paso |
|---|---|---|---|
| Git | **En progreso** | Rama confirmada: `feature/juanfer`. Antes de preparar el commit había cambios en `.idea/`, la guía DOCX, el resumen y cuatro archivos nativos; `backend/planificador-service/` era nuevo. Solo el resumen actualizado y los cuatro archivos del microservicio se preparan para este commit. | Mantener fuera los cambios anteriores cuyo origen no corresponde a esta tarea y verificar el push. |
| Estructura base | **En progreso** | Existen `frontend/`, `backend/`, `native/`, `infra/`, `tests/` y `docs/`. `backend/planificador-service` tiene el esqueleto inicial; `proceso-service`, `frontend/` y la carpeta superior `tests/` siguen sin implementación. | Continuar por etapas sin crear un tercer servicio ni adelantar contratos pendientes. |
| Documentación | **En progreso** | Están README, ADD, bitácora, fuentes originales, wireframes y este resumen. README y ADD aún expresan algunas decisiones como pendientes o sin resolver que la bitácora ya confirma. | Revisar esa deriva en una tarea documental autorizada; preservar las fuentes originales. |
| OpenAPI | **En progreso** | `docs/openapi.yaml` declara 9 operaciones. YAML parseado; 19 esquemas, 6 respuestas y referencias internas comprobadas; 9 `operationId` únicos. La mayoría de rutas/campos son propuestas pendientes de validación. | Revisar y aprobar el contrato con el equipo antes de implementarlo. No hay endpoints implementados. |
| Docker Compose y SQL | **En progreso** | Compose contiene solo `postgres` y `keycloak`; `docker compose config --quiet` pasó. Hay SQL de inicialización y `.env.example`. El `.env` local no existe. | Iniciar Docker Desktop y comprobar la base/Keycloak; ejecutar y verificar el SQL. No se ha hecho. |
| FCFS/FIFO nativo | **Completado** | Los cuatro archivos de `native/` implementan FCFS/FIFO y casos de prueba; `make -C native clean` seguido de `make -C native test` pasó en Ubuntu/WSL y mostró `OK: pruebas FCFS/FIFO`. El ejecutable se limpió después. | Conservar el resultado como evidencia de prueba local; aún no representa RF-03 completo ni aceptación oficial del profesor. |
| Núcleo C completo | **En progreso** | FCFS/FIFO, ABI `planificador_fcfs_v1()` y objetivo `library` están implementados; los resultados exitosos de las ejecuciones previas son reportados. SJF, SRTF y Round Robin siguen pendientes. | Continuar según contratos y oráculos aprobados, sin modificar la lógica FIFO. |
| Integración JNA | **Pendiente** | Existe el proyecto base Java, pero aún no tiene JNA, estructuras mapeadas ni adaptador. | Confirmar el contrato C/JNA y el entorno compatible; luego probar la integración aislada. |
| Microservicios | **En progreso** | `planificador-service` tiene una estructura inicial Maven/Spring Boot; `proceso-service` aún no existe. No hay endpoints ni funcionalidades implementados. | Continuar el servicio por etapas, respetando los contratos que requieren aprobación. |
| Seguridad Keycloak/JWT | **Pendiente** | No existe realm de aplicación ni configuración de clientes/resource servers. Compose solo prepara el servicio Keycloak. | Crear/importar realm y validar autenticación/autorización en ambos servicios en su fase. |
| Frontend Angular | **Pendiente** | `frontend/` está vacío; no hay manifiestos ni componentes. | Implementar tras acordar API y flujos de autenticación; obtener catálogo desde la API, sin lista fija. |
| Persistencia e historial | **Pendiente** | Compose/SQL proponen bases y roles separados, pero no están ejecutados; no hay esquema ni código de persistencia. | Verificar la infraestructura y acordar campos/retención del historial antes de persistirlo en `planificador-service`. |
| Pruebas | **En progreso** | En esta revisión, `mvn -f backend/planificador-service/pom.xml test` terminó con BUILD SUCCESS: 1 prueba, 0 fallos, 0 errores, 0 omitidas. La aplicación también arrancó desde la configuración Spring Boot de IntelliJ; se confirmó un listener en 8080 y una respuesta HTTP 404 en `/`, ya que aún no hay rutas. Las advertencias de Mockito/agentes dinámicos no impidieron la prueba. No se repitieron las pruebas C. | Añadir pruebas y verificaciones conforme se implementen contratos, JNA y endpoints. |
| Benchmarks | **Pendiente** | No hay scripts, datos, resultados ni mediciones. Solo existen protocolos documentados. | Implementar scripts separados y registrar resultados reproducibles al disponer de motores equivalentes. |

**Verificación repetida el 2026-10-21:** desde WSL se ejecutaron `make -C native clean`, `make -C native test` y una limpieza posterior. GCC compiló con `-std=c11 -Wall -Wextra -Werror -pedantic -O2`; los casos pasaron y la salida final fue `OK: pruebas FCFS/FIFO`, con segmentos Gantt y métricas impresos. El binario generado se eliminó con `make clean`. Esto verifica el código y prueba local de FCFS/FIFO en ese entorno; no verifica `libplanificador.so`, JNA, HTTP, microservicios, contenedores ni otros algoritmos. Compose/OpenAPI no se volvieron a ejecutar.

## 6. Roadmap de trabajo

El roadmap conserva las etapas y dependencias de la guía maestra. Las propuestas de orden se limitan a adelantar verificaciones de integración para descubrir bloqueos temprano.

### Fase 0 — Estructura base y Git

- **Completado:** Git inicializado y `.gitignore` de raíz presente.
- **En progreso:** existen las carpetas superiores `frontend/`, `backend/`, `native/`, `infra/`, `tests/` y `docs/`, pero la estructura base todavía está incompleta.
- **Pendiente:** crear las carpetas `backend/proceso-service/` y `backend/planificador-service/`; no se necesitan más subcarpetas nativas para la tarea actual.
- **Criterio de cierre:** las carpetas previstas existen con nombres correctos y los cambios locales se han revisado. Esto cierra la preparación de estructura, no la implementación funcional.

### Fase 1 — Contratos y entorno local

- **Completado:** existe un borrador OpenAPI; Compose y SQL de inicialización están escritos; configuración estática de Compose pasó.
- **Pendiente:** validar OpenAPI semánticamente y aprobar rutas/esquemas; resolver puntos abiertos del ABI C/JNA; iniciar Docker, ejecutar SQL y verificar persistencia y conexión Keycloak/PostgreSQL; no existe realm importado.
- **Criterio de cierre:** contratos revisados y congelados por los responsables; PostgreSQL y Keycloak locales comprobados desde un arranque limpio. El Compose actual no incorpora aplicaciones.

### Fase 2 — Núcleo nativo

- **Completado:** requisitos y algoritmo base identificados en los documentos.
- **En progreso:** FCFS/FIFO tiene fuente, cabecera y casos de prueba locales. El orden FIFO se confirmó: llegada primero; en igual llegada, prioridad ascendente y luego orden original. Esta interfaz nativa aún no constituye el ABI JNA aprobado.
- **Completado y verificado:** FCFS/FIFO local y su prueba reproducible pasaron con GCC/Make en WSL.
- **Pendiente:** contrato ABI, tamaños/límites, gestión de memoria, códigos de error, reglas de desempate para SJF/SRTF/RR, oráculos oficiales, biblioteca `libplanificador.so`, algoritmos SJF/SRTF/RR y sus suites.
- **Criterio de cierre:** FIFO, SJF, SRTF y Round Robin coinciden en segmentos completos y métricas con oráculos manuales; pruebas nativas independientes ejecutables en menos de 30 segundos.

### Fase 3 — Integración Java/JNA

- **Completado:** solo está definida la dirección arquitectónica.
- **Pendiente:** interfaz JNA, mapeo de estructuras, carga de biblioteca, adaptador aislado y motor Java equivalente seleccionable por configuración.
- **Criterio de cierre:** una corrida validada de punta a punta Java → C → resultado; errores nativos traducidos y resultados de referencia Java/C verificados.

### Fase 4 — Microservicios y persistencia

- **Completado:** responsabilidades y número de servicios están confirmados.
- **Pendiente:** implementar los dos servicios, API aprobada, PostgreSQL de cada servicio, persistencia de conjuntos e historial propio de `planificador-service`.
- **Criterio de cierre:** operaciones aprobadas y persistencia sobreviven reinicio; historial conserva el `sub` autenticado y cada usuario solo ve sus corridas.

### Fase 5 — Seguridad

- **Completado:** Keycloak, roles y validación por microservicio están definidos como dirección.
- **Pendiente:** archivo de importación de realm, clientes, roles/usuarios de prueba, Resource Server y reglas de autorización.
- **Criterio de cierre:** evidencia reproducible de 401 sin token y token inválido, 403 con rol incorrecto, éxito con rol correcto, expiración y cierre de sesión según requisitos.

### Fase 6 — Frontend

- **Completado:** requisitos y wireframes están disponibles.
- **Pendiente:** aplicación Angular, OIDC/PKCE, interceptor, formularios, catálogo, comparación, Gantt, métricas y acceso al historial.
- **Criterio de cierre:** interfaz usa contratos aprobados, presenta valores del backend sin recalcular promedios, satisface legibilidad y prueba de usabilidad requeridas.

### Fase 7 — Validación y rendimiento

- **Completado:** protocolos Web 2 y Arquitectura están separados en la bitácora.
- **Pendiente:** scripts, pruebas nativas/JUnit/integración, benchmarks, resultados y sesiones de usabilidad.
- **Criterio de cierre:** pruebas manuales de control y suites reproducibles; benchmarks separados con las cargas y repeticiones acordadas; resultados registrados sin mezclar metodologías.

### Fase 8 — Despliegue y documentación final

- **Completado:** Compose local base y objetivo Docker Swarm están definidos; el Compose base no ha arrancado.
- **Pendiente:** Dockerfile multietapa del planificador, stack completo Swarm, límites de CPU/memoria, secretos apropiados, arranque desde cero, mediciones de imagen/arranque y evidencias finales.
- **Criterio de cierre:** despliegue limpio con un comando, imagen nativa sin compilador, sistema listo dentro de los umbrales documentados y documentación/evidencias verificables.

**Recomendación de integración:** adelantar el arranque de la infraestructura local y una cadena vertical mínima tan pronto como los contratos estén aprobados, sin alterar el orden exigido de escribir los casos manuales, probar primero C independientemente y validar una llamada JNA antes de completar los demás algoritmos.

## 7. Contratos y funcionalidades pendientes

`docs/openapi.yaml` es un borrador de API, no una implementación ni un contrato completamente aprobado. Contiene estas operaciones:

| Servicio/tag | Operación declarada | Estado contractual |
|---|---|---|
| `planificador-service` | `GET /algoritmos` | Endpoint mínimo requerido. El catálogo propone objetos con identificador abierto; aún debe validarse el formato exacto. |
| `planificador-service` | `POST /planificaciones` | Endpoint mínimo requerido. Propone carga completa de procesos, algoritmo y quantum opcional; ejecución, almacenamiento y respuesta aún no implementados. |
| `planificador-service` | `POST /comparaciones` | Propuesta para comparar una carga con una sola acción. Selección de algoritmos y asociación con historial por validar. |
| `planificador-service` | `GET /historial` | Propuesta de listado limitado al `sub` autenticado; respuesta y paginación por validar. |
| `planificador-service` | `GET /historial/{planificacionId}` | Propuesta de detalle de corrida propia; formato de identificador y semántica para corrida ajena/inexistente por validar. |
| `proceso-service` | `GET /conjuntos` | Propuesta de listado; formato y paginación por validar. |
| `proceso-service` | `POST /conjuntos` | Propuesta de registro de conjunto de referencia, con autorización docente. La distinción entre conjuntos de referencia y personales queda por revisar. |
| `proceso-service` | `GET /conjuntos/{conjuntoId}` | Propuesta de consulta individual. |
| `proceso-service` | `DELETE /conjuntos/{conjuntoId}` | Propuesta de eliminación de conjunto de referencia, permitida a docente. |

La especificación declara bearer JWT a nivel global y esquemas reutilizables para procesos, segmentos de ejecución, intervalos ociosos, métricas, promedio, historial y errores. No enumera algoritmos de forma cerrada. Las horas/segmentos, respuestas HTTP, campos de conjunto/historial, formato de PID frente a los wireframes, comportamiento de comparación y errores siguen sujetos a revisión. **Ninguna de estas rutas está implementada.**

La ABI `planificador_fcfs_v1()` con tipos fijos ya está implementada en C. La integración Java/JNA sigue pendiente: aún hay que verificar el mapeo exacto de estructuras/alineación, las llamadas y capacidades, la traducción de errores y la carga de la biblioteca desde el entorno Java compatible.

## 8. Pruebas y benchmarks

- Se creó `native/tests/test_planificador.c` con casos de desarrollo para FCFS/FIFO: llegadas distintas y empatadas, prioridad y orden de entrada en empate de llegada, CPU ociosa al inicio y entre procesos, un proceso, capacidad insuficiente y ráfaga inválida. Los resultados esperados se calcularon para esta prueba, no proceden de documentos del profesor.
- Verificación reportada en Ubuntu/WSL el 2026-10-21: `make -C native clean` y `make -C native test` pasaron con GCC; se imprimieron el Gantt, periodos ociosos, métricas y `OK: pruebas FCFS/FIFO`. En una ejecución reportada posteriormente también pasaron `make -C native library`; en esta tarea no se repitieron las pruebas ni la generación.
- No hay casos manuales oficiales versionados ni pruebas de integración. La prueba básica de carga de contexto de `planificador-service` sí se ejecutó el 2026-10-22 y pasó: 1 prueba, 0 fallos, 0 errores, 0 omitidas.
- No se encontraron scripts de benchmark, datos, resultados crudos, medianas ni informes de usabilidad.
- No se ejecutaron pruebas de integración, aceptación de los ocho requisitos ni benchmarks en esta revisión; las pruebas nativas no se repitieron.
- La única validación de infraestructura observada es el parseo de configuración de Compose, que pasó sin iniciar contenedores.
- La sintaxis YAML de OpenAPI pudo parsearse con el parser disponible; sus referencias internas (19 esquemas y 6 respuestas) se resolvieron y los 9 `operationId` son únicos. Esto no equivale a validar el documento con un validador OpenAPI dedicado.
- El protocolo de Web 2 está especificado como 50, 500, 5.000 y 50.000 procesos, 30 repeticiones y mediana.
- El protocolo de Arquitectura está especificado como SRTF con 10.000 procesos, cinco ejecuciones consecutivas y descarte de tres por calentamiento JIT. No hay resultados. La cantidad de ejecuciones útiles después del descarte queda en dos, conforme a la decisión de equipo; los dos protocolos no deben mezclarse.

## 9. Problemas, riesgos y decisiones pendientes

1. **Reglas concretas de prioridad por algoritmo:** FCFS/FIFO ya tiene política confirmada para esta etapa: llegada primero; con igual llegada, menor prioridad numérica y luego orden original. Falta especificar la aplicación en empates de ráfaga para SJF, empates de tiempo restante para SRTF y orden/reingreso de Round Robin, sin convertirlos implícitamente en planificación por prioridad.
2. **Integración C/JNA:** ABI v1 y sus códigos están definidos en C; el mapeo JNA, las verificaciones de estructura, la carga de biblioteca y las pruebas Java↔C siguen pendientes.
3. **OpenAPI:** endpoints, esquemas y códigos propuestos requieren aprobación; la comprobación realizada no fue una validación OpenAPI formal. El borrador ya propone HTTP 400 con `application/json` y `ApiError`, pero no está aprobado ni probado. `POST /conjuntos` está descrito como registro de conjuntos de referencia solo por docente, mientras RF-02 pide guardar/recuperar conjuntos de prueba con nombre; confirmar si se trata del mismo tipo de conjunto, sus permisos y endpoints.
4. **Historial:** ubicación y propiedad por usuario están confirmadas; aún falta acordar campos exactos, retención, snapshots y semántica al pedir una corrida ajena.
5. **Stack:** siguen pendientes elecciones exactas dentro de los rangos acordados (Spring Boot, Angular, JNA y C11/C17).
6. **Documentación desactualizada:** `README.md` todavía dice que la discrepancia de dos/tres servicios está pendiente y lista versiones y decisiones ya resueltas como pendientes. `docs/arquitectura-add.md` también mantiene como pendientes el número de servicios, prioridad, origen de procesos, historial y orquestador. No se modificaron en esta tarea; corregirlos requiere una tarea documental separada.
7. **Infraestructura no ejecutada:** el Compose pasa validación estática, pero `docker compose ps` no pudo conectar con Docker Desktop (`dockerDesktopLinuxEngine` no disponible). No hay `infra/.env` local; SQL, Keycloak, persistencia y salud en ejecución no están verificados.
8. **Deriva Git:** el árbol tenía cambios locales y archivos no rastreados. Revisar antes de preparar cambios o commits; en particular, no eliminar la configuración `.idea/inspectionProfiles/` sin conocer su procedencia.
9. **Aceptación de persistencia:** Compose define el volumen nombrado `postgres_data`, pero no incluye `proceso-service`; la prueba RF-01 de recuperar cinco procesos tras reiniciar el contenedor todavía requiere implementar/integrar el servicio y ejecutar Docker.
10. **Comparación dinámica:** OpenAPI propone `POST /comparaciones` con una lista `algorithmIds`. RF-07 pide comparar todos los algoritmos con la misma entrada; implementar el consumo del catálogo dinámico y aprobar si el servicio recibe todos los identificadores explícitamente o los resuelve desde el catálogo, sin listas cerradas en Angular.

La decisión de exactamente dos microservicios está confirmada por el profesor y **no se reabre como tarea de arquitectura**. La regla de prioridad de tres niveles también está confirmada; solo resta especificar su aplicación por algoritmo.

## 10. Próxima tarea recomendada

**Definir y aprobar el contrato mínimo C/JNA y construir `libplanificador.so` sin cambiar la política FCFS/FIFO.**

La prueba local de FCFS/FIFO ya pasó en WSL. Antes de integrar Java, el equipo aún debe aprobar firmas, tipos/alineación, límites, propiedad y liberación de memoria y códigos de error para JNA. La interfaz actual de `planificador.h` es de prueba local, no un ABI aprobado.

- **Archivo/componente:** contrato nativo en `native/include/planificador.h` y futura configuración de compilación de `libplanificador.so`; especificación de mapeo JNA aislada en el futuro `planificador-service`.
- **Resultado esperado:** acuerdo documentado que permite una llamada C→Java sin ambigüedad de estructura, capacidad, memoria o error. La generación de la biblioteca y las llamadas JNA se implementarán/verificarán después del acuerdo.
- **Cómo comprobarlo:** revisión conjunta de una firma y estructuras concretas, compilación de biblioteca compartida en Linux y prueba aislada desde Java usando JNA; mantener pasando `make -C native test`.
- **No modificar durante esta tarea:** la política y pruebas FCFS/FIFO ya verificadas, documentos originales, decisiones de dos microservicios y sin API Gateway; no implementar otro algoritmo ni frontend o persistencia en el mismo cambio.

## 11. Instrucciones para el siguiente agente de IA

1. Leer primero `docs/resumen-contexto.md`.
2. Leer `docs/bitacora-decisiones.md` antes de proponer cambios que puedan afectar decisiones arquitectónicas.
3. Revisar los archivos reales antes de asumir que una funcionalidad existe.
4. No modificar decisiones confirmadas sin autorización.
5. No crear un tercer microservicio ni añadir un API Gateway.
6. No reescribir la bitácora para registrar el avance cotidiano.
7. No sobrescribir archivos ni borrar datos, volúmenes o configuraciones sin explicar el riesgo y obtener autorización.
8. Trabajar paso a paso, con cambios pequeños y verificables.
9. No afirmar que una prueba, compilación o integración funciona si no se ha ejecutado y comprobado.
10. Al terminar cada tarea importante, actualizar el estado de este resumen y recomendar el siguiente paso.

### Prompt de recuperación

> Estoy trabajando en el repositorio `los-naufragos`. Lee primero `docs/resumen-contexto.md` y después consulta la bitácora si necesitas conocer las razones de las decisiones. Verifica el estado real de los archivos y dime cuál es la siguiente tarea recomendada. Explícame cómo realizarla paso a paso y no modifiques archivos sin explicarme primero qué vas a cambiar

## Backlog funcional de Jira

Este resumen organiza los ocho requisitos funcionales proporcionados por el equipo. No se confirmó acceso directo a Jira ni se verificó el estado de sus tickets. RF-03 está **En progreso**: el núcleo FCFS/FIFO local está implementado y su prueba pasó, pero faltan la biblioteca compartida, JNA, `POST /planificaciones` y aceptación de extremo a extremo. Los otros siete requisitos están **Pendiente**.

| ID / requisito | Objetivo | Componentes implicados | Criterio principal de aceptación | Estado |
|---|---|---|---|---|
| RF-01 — Registrar y persistir procesos/conjuntos | Registrar procesos con identificadores únicos dentro del conjunto y persistirlos. | `frontend/`, `proceso-service`, PostgreSQL, Compose. | Recuperar un conjunto de cinco procesos después de reiniciar el contenedor y comprobar que los datos se conservan. | **Pendiente** |
| RF-02 — Guardar y recuperar conjuntos de prueba con nombre | Guardar un conjunto nombrado y recuperarlo usando los endpoints de conjuntos correspondientes. | `frontend/`, API de `proceso-service`, PostgreSQL. | Guardar y volver a cargar el mismo conjunto por su nombre mediante los endpoints acordados. | **Pendiente** |
| RF-03 — Ejecutar FIFO en C mediante JNA | Ejecutar `POST /planificaciones` con FIFO en C integrado desde Java mediante JNA. | `planificador-service`, `libplanificador.so`, JNA, validación del JWT. | Los tiempos de los segmentos devueltos coinciden exactamente con el diagrama de Gantt de referencia. El FIFO C local está probado; HTTP/JNA no están implementados. | **En progreso** |
| RF-04 — Quantum configurable para Round Robin | Permitir cambiar el quantum en cada corrida sin compilar de nuevo. | `frontend/`, `planificador-service`, API, núcleo C/JNA. | Ejecutar la misma entrada con quantum 2 y 5 y comparar resultados sin recompilar. | **Pendiente** |
| RF-05 — Diagrama de Gantt con ociosidad | Mostrar segmentos de ejecución y periodos de CPU ociosa. | `frontend/`, respuesta de `planificador-service`. | Visualizar ocho procesos a 1366 px sin desplazamiento horizontal, distinguiendo cada segmento y los periodos ociosos. | **Pendiente** |
| RF-06 — Métricas por proceso y promedios | Calcular espera, retorno y respuesta por proceso y sus promedios en el backend. | `planificador-service`, núcleo/motor de planificación, `frontend/`. | Verificar los valores y promedios con resultados manuales; Angular presenta, no calcula los promedios. | **Pendiente** |
| RF-07 — Comparar algoritmos sobre la misma entrada | Ejecutar todos los algoritmos con la misma entrada y comparar sus métricas promedio. | `planificador-service`, catálogo dinámico, núcleo C/JNA o motor Java, `frontend/`. | Mostrar la comparación común e identificar el algoritmo con mejor métrica promedio según el criterio aprobado. | **Pendiente** |
| RF-08 — Validar entradas con errores HTTP 400 JSON | Validar ráfaga > 0, llegada ≥ 0 y quantum ≥ 1. | Validación en Angular y autoritativa en `planificador-service`/API. | Cada dato inválido responde HTTP 400 con JSON de error; el formato exacto de ese JSON debe quedar en el contrato aprobado. | **Pendiente** |

**Base de los criterios:** *Los Náufragos – Planificador de procesos*, PDF, pp. 5–6, RF-01 a RF-08. Los componentes se asignan según las responsabilidades arquitectónicas confirmadas en este resumen. CPU ociosa se devuelve como parte de la línea de tiempo; no se añade un servicio para ello.

### Secuencia recomendada para implementar el backlog

**Estado de las condiciones previas:** la regla concreta de FCFS/FIFO fue confirmada y el núcleo local pasó sus pruebas creadas para desarrollo. El contrato C/JNA, los oráculos oficiales y el contrato HTTP siguen pendientes; no se atribuyen los valores de desarrollo al profesor ni se consideran aceptación de RF-03.

1. **Núcleo C: FCFS/FIFO — Implementado y verificado localmente.** `native/include/planificador.h`, `native/src/planificador.c`, `native/tests/test_planificador.c` y `native/Makefile` compilaron y pasaron en WSL. No hay biblioteca compartida verificada.
2. **Completar la suite del núcleo.** Añadir SJF, SRTF y Round Robin al núcleo y pruebas de referencia de la línea de tiempo completa; comprobar llegada, CPU ociosa, quantum y desempates según los casos aprobados.
3. **Integración Java/JNA.** Mapear el ABI acordado en una interfaz aislada y validar un caso de extremo a extremo C→JNA; implementar el motor Java de referencia y comprobar equivalencia funcional.
4. **`planificador-service`.** Implementar el adaptador de motor, validación/backend y las operaciones de planificación y catálogo según el OpenAPI revisado; calcular promedios en backend y persistir historial asociado al `sub`.
5. **`proceso-service` y persistencia de conjuntos.** Administrar procesos y conjuntos, validar unicidad y entradas, y verificar persistencia tras reinicio. Una corrida individual sigue recibiendo la carga completa de Angular y no depende de que esté guardada.
6. **Seguridad e integración de interfaz.** Configurar Keycloak/JWT y permisos en ambos servicios; conectar Angular para capturar/cargar datos, seleccionar algoritmos desde el catálogo dinámico, ejecutar/comparar, presentar Gantt y métricas, y comunicar validaciones.
7. **Aceptación funcional y evidencias.** Ejecutar los ocho criterios del backlog, casos manuales y pruebas de integración. Mantener separados los dos protocolos de benchmark. Registrar resultados únicamente después de ejecutarlos.

La secuencia mantiene exactamente `proceso-service` y `planificador-service`, sin API Gateway. El orden C→JNA→servicio→interfaz sigue la guía maestra. La política confirmada para FCFS/FIFO se aplica únicamente a este algoritmo; no fija por sí sola los desempates de SJF, SRTF ni Round Robin.

## Estado actual y siguiente paso

**Revisión del repositorio: 2026-10-22.** Esta sección actualiza el estado indicado en los apartados anteriores cuando haya diferencias.

### Arquitectura acordada

- Frontend Angular.
- Exactamente dos microservicios Spring Boot: `proceso-service` y `planificador-service`.
- Biblioteca nativa en C llamada desde Java mediante JNA.
- Keycloak para autenticación.
- No introducir un API Gateway.

### Trabajo nativo realizado

- FCFS/FIFO está implementado en C y dispone de pruebas.
- Se añadió el adaptador ABI `planificador_fcfs_v1()` con tipos de tamaño fijo basados en `int32_t` e `int64_t`.
- `native/Makefile` dispone del objetivo `library` para generar `native/libplanificador.so`, separado del ejecutable de pruebas.
- Las ejecuciones previas reportaron que `make -C native test` y `make -C native library` finalizaron correctamente. No se repitieron las pruebas nativas ni la generación de la biblioteca en esta sesión.
- En la inspección actual, `native/libplanificador.so` existe, es ELF Linux x86-64 y exporta `planificador_fcfs` y `planificador_fcfs_v1`. La inspección de presencia y símbolos no sustituye la repetición de las pruebas.

### Situación de la integración Java

- `backend/planificador-service` contiene `pom.xml`, `PlanificadorServiceApplication.java`, `PlanificadorServiceApplicationTests.java` y `README.md`. El POM declara Java 21 y Spring Boot 3.5.7.
- El 2026-10-22, Maven 3.9.16 incluido con IntelliJ ejecutó `mvn -f backend/planificador-service/pom.xml test`: BUILD SUCCESS, 1 prueba, 0 fallos, 0 errores y 0 omitidas. Las advertencias de Mockito/agentes dinámicos no impidieron el éxito.
- La configuración Spring Boot de IntelliJ inició la aplicación; se verificó el puerto 8080 escuchando y una respuesta HTTP 404 en `/`, esperable porque aún no hay ruta raíz. Se detuvo la sesión después de comprobarlo. Un intento separado de `mvn spring-boot:run` falló al descargar dependencias del plugin por resolución DNS de Maven Central.
- No hay endpoints funcionales de planificación, algoritmos Java ni integración JNA. JNA sigue pendiente.
- La biblioteca disponible es Linux x86-64. Antes de cargarla desde Java hay que confirmar el entorno de ejecución y usar un entorno/plataforma compatible.

### Siguiente paso acordado

1. Verificar el estado Git y localizar cambios previos antes de preparar nuevos commits.
2. Revisar y aprobar el contrato REST de `docs/openapi.yaml` antes de implementar endpoints.
3. Integrar JNA en `planificador-service`, mapeando cuidadosamente estructuras y ABI de C.
4. Resolver cómo se cargará la biblioteca nativa según el entorno de ejecución; la biblioteca disponible es Linux x86-64.
5. Probar la llamada Java↔C sin cambiar la lógica FIFO.
6. Continuar con los algoritmos y endpoints restantes conforme a los requisitos del proyecto.

### Archivos del esqueleto inicial del microservicio

- `backend/planificador-service/pom.xml`: configuración Maven, Java 21, Spring Boot 3.5.7, Spring Web y pruebas.
- `backend/planificador-service/src/main/java/com/losnaufragos/planificador/PlanificadorServiceApplication.java`: punto de entrada Spring Boot.
- `backend/planificador-service/src/test/java/com/losnaufragos/planificador/PlanificadorServiceApplicationTests.java`: prueba básica de carga del contexto.
- `backend/planificador-service/README.md`: requisitos y comandos de prueba/ejecución.

### Estado Git observado antes del commit de esta actualización

- Rama confirmada por `git branch --show-current`: `feature/juanfer`.
- Los cambios de `.idea/` (tres archivos añadidos al índice y `misc.xml` modificado), `docs/Guia_maestra_Copilot_Planificador_Los_Naufragos.docx` y los cuatro archivos de `native/` ya estaban presentes antes de preparar este commit. Se dejan fuera; no se atribuyen automáticamente a esta sesión.
- Los únicos archivos seleccionados para este commit son este resumen actualizado y los cuatro archivos del esqueleto `backend/planificador-service/`, creados en la tarea anterior. No se incluyen `target/`, binarios, bibliotecas nativas ni secretos.
