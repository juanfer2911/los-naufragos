# Resumen de contexto y continuidad

**Proyecto:** Los Náufragos — Planificador de procesos PI  
**Última revisión del estado real:** 2026-10-20  
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
- **Prioridad:** es un atributo numérico, no un algoritmo adicional. Menor número significa mayor prioridad. Regla de desempate confirmada: primero menor número de prioridad; si empatan, menor tiempo de llegada; si también empatan, se respeta el orden original de entrada. Sigue pendiente concretar su aplicación en los puntos de decisión de cada algoritmo sin alterar su política propia.
- **Entorno:** Docker Compose es para integración local; Docker Swarm es el objetivo de despliegue. No crear manifiestos de Kubernetes.
- **Benchmarks:** son suites independientes con scripts separados en `tests/`:
  - Web 2: cargas de 50, 500, 5.000 y 50.000 procesos, 30 repeticiones por carga y mediana del tiempo de respuesta.
  - Arquitectura de Computadores: SRTF con 10.000 procesos, cinco ejecuciones consecutivas y descarte de las primeras tres por calentamiento JIT. Quedan dos ejecuciones posteriores al descarte, según la decisión confirmada; no mezclar las metodologías.

La decisión de dos servicios no se considera pendiente aunque el enunciado general mencione un mínimo de tres. La bitácora registra que el profesor confirmó los dos para este proyecto.

## 3. Estructura real del repositorio

Árbol relevante observado. Las carpetas que aparecen vacías son estructura existente, no componentes implementados.

```text
.
├── backend/                         # Vacía; aún no existen las carpetas de servicio
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
├── native/                          # Vacía; no hay include/, src/ ni tests/
├── tests/                           # Vacía; no hay suites ni scripts
├── .gitignore
└── README.md
```

**No existen todavía** `backend/proceso-service/`, `backend/planificador-service/`, `native/include/`, `native/src/` ni `native/tests/`. No se encontraron archivos `.gitkeep`. La búsqueda encontró cero archivos en `frontend/`, `backend/`, `native/` y `tests/`.

Git sí está inicializado en la rama `main`, que sigue a `origin/main`. Al revisar, el árbol de trabajo no estaba limpio: `docs/bitacora-decisiones.md` aparecía modificada, `docs/openapi.yaml` e `infra/` como no rastreados, y `.idea/inspectionProfiles/` como contenido no rastreado del IDE. No se debe atribuir ni descartar esos cambios sin revisar su procedencia.

## 4. Tecnologías y versiones

| Tecnología | Estado según archivos revisados |
|---|---|
| Angular 17 o superior, standalone y signals | Rango acordado y documentado en la bitácora. No hay `package.json`, fuentes ni proyecto Angular; versión instalada/configurada **no verificada**. |
| Java 21 LTS | Versión acordada en la bitácora. No hay build del backend ni configuración de proyecto; uso por los servicios **no verificado**. |
| Spring Boot 3.2 o superior compatible con Java 21 | Rango acordado; no hay `pom.xml`, `build.gradle` ni microservicios. Versión exacta **pendiente**. |
| C con GCC, estándar C11 o C17 | Alternativas/rango acordados; no hay fuente, Makefile ni biblioteca. Estándar elegido y compilación **pendientes**. |
| JNA 5.x | Rango acordado; no hay dependencia Java configurada ni interfaz JNA. Versión exacta **pendiente**. |
| PostgreSQL 16 | Versión mayor acordada. `infra/docker-compose.yml` fija `postgres:16.4`; el archivo pasa la validación estática de Compose, pero contenedor y base **no verificados en ejecución**. |
| Keycloak 24 o superior | Mínimo acordado. Compose fija `quay.io/keycloak/keycloak:25.0.6`, conectado a PostgreSQL. No se comprobó que el contenedor iniciara ni existe importación del realm de aplicación. |
| Docker Compose / Docker Swarm | Compose está definido para la infraestructura local base; Swarm está confirmado como objetivo futuro. No hay stack Swarm ni despliegue completo. |

Las versiones acordadas en la bitácora no implican dependencias instaladas ni proyectos compilables. Las etiquetas de imágenes son configuración declarada, no evidencia de que el entorno se haya desplegado correctamente.

## 5. Estado actual del proyecto

Estados utilizados en la tabla: **Completado**, **En progreso**, **Pendiente** y **Por verificar**. “Completado” se aplica solo a lo comprobado en archivos o pruebas, no por la mera existencia de un diseño.

| Área o tarea | Estado | Evidencia | Siguiente paso |
|---|---|---|---|
| Git | **Completado** | `git rev-parse` confirma repositorio; rama `main` con upstream `origin/main`. El árbol tenía cambios locales/no rastreados al revisarlo. | Revisar procedencia y alcance de los cambios antes de preparar cualquier commit. |
| Estructura base | **En progreso** | Existen `frontend/`, `backend/`, `native/`, `infra/`, `tests/` y `docs/`; las cuatro áreas de código/pruebas están vacías. Faltan carpetas de servicios y subcarpetas nativas previstas. | Crear la estructura restante cuando corresponda, sin confundir carpetas con implementación. |
| Documentación | **En progreso** | Están README, ADD, bitácora, fuentes originales, wireframes y este resumen. README y ADD aún expresan algunas decisiones como pendientes o sin resolver que la bitácora ya confirma. | Revisar esa deriva en una tarea documental autorizada; preservar las fuentes originales. |
| OpenAPI | **En progreso** | `docs/openapi.yaml` declara 9 operaciones. YAML parseado; 19 esquemas, 6 respuestas y referencias internas comprobadas; 9 `operationId` únicos. La mayoría de rutas/campos son propuestas pendientes de validación. | Revisar y aprobar el contrato con el equipo antes de implementarlo. No hay endpoints implementados. |
| Docker Compose y SQL | **En progreso** | Compose contiene solo `postgres` y `keycloak`; `docker compose config --quiet` pasó. Hay SQL de inicialización y `.env.example`. El `.env` local no existe. | Iniciar Docker Desktop y comprobar la base/Keycloak; ejecutar y verificar el SQL. No se ha hecho. |
| Núcleo C | **Pendiente** | `native/` no contiene archivos; no hay cabecera, fuentes, pruebas ni `libplanificador.so`. | Cerrar reglas concretas de empate y contrato ABI, preparar oráculos manuales y luego comenzar por FIFO. |
| Integración JNA | **Pendiente** | No hay proyecto Java, dependencia JNA, estructuras mapeadas ni adaptador. | Implementar después de acordar el contrato C/JNA y validar una corrida nativa independiente. |
| Microservicios | **Pendiente** | `backend/` está vacío; no existen los dos proyectos Spring Boot. | Crear exactamente `proceso-service` y `planificador-service` cuando se aprueben contratos y versiones. |
| Seguridad Keycloak/JWT | **Pendiente** | No existe realm de aplicación ni configuración de clientes/resource servers. Compose solo prepara el servicio Keycloak. | Crear/importar realm y validar autenticación/autorización en ambos servicios en su fase. |
| Frontend Angular | **Pendiente** | `frontend/` está vacío; no hay manifiestos ni componentes. | Implementar tras acordar API y flujos de autenticación; obtener catálogo desde la API, sin lista fija. |
| Persistencia e historial | **Pendiente** | Compose/SQL proponen bases y roles separados, pero no están ejecutados; no hay esquema ni código de persistencia. | Verificar la infraestructura y acordar campos/retención del historial antes de persistirlo en `planificador-service`. |
| Pruebas | **Pendiente** | `tests/` y `native/` no contienen pruebas ni scripts; no se encontraron suites JUnit o C. | Crear oráculos manuales y las suites independientes según requisitos. |
| Benchmarks | **Pendiente** | No hay scripts, datos, resultados ni mediciones. Solo existen protocolos documentados. | Implementar scripts separados y registrar resultados reproducibles al disponer de motores equivalentes. |

**Verificaciones ejecutadas:** la configuración de Compose pasa `docker compose config --quiet`; los servicios declarados son solo PostgreSQL y Keycloak. El YAML de OpenAPI se pudo parsear con el parser YAML disponible y se revisaron referencias internas y unicidad de `operationId`; no había validador OpenAPI dedicado. `docker compose ps` no pudo conectarse al motor Docker Desktop (`dockerDesktopLinuxEngine` no disponible). No se ejecutaron contenedores, SQL, pruebas funcionales, compilaciones ni benchmarks.

## 6. Roadmap de trabajo

El roadmap conserva las etapas y dependencias de la guía maestra. Las propuestas de orden se limitan a adelantar verificaciones de integración para descubrir bloqueos temprano.

### Fase 0 — Estructura base y Git

- **Completado:** Git inicializado y `.gitignore` de raíz presente.
- **En progreso:** existen las carpetas superiores `frontend/`, `backend/`, `native/`, `infra/`, `tests/` y `docs/`, pero la estructura base todavía está incompleta.
- **Pendiente:** crear las carpetas acordadas `backend/proceso-service/`, `backend/planificador-service/`, `native/include/`, `native/src/` y `native/tests/`; añadir `.gitkeep` solo si se necesita versionar carpetas vacías.
- **Criterio de cierre:** las carpetas previstas existen con nombres correctos y los cambios locales se han revisado. Esto cierra la preparación de estructura, no la implementación funcional.

### Fase 1 — Contratos y entorno local

- **Completado:** existe un borrador OpenAPI; Compose y SQL de inicialización están escritos; configuración estática de Compose pasó.
- **Pendiente:** validar OpenAPI semánticamente y aprobar rutas/esquemas; resolver puntos abiertos del ABI C/JNA; iniciar Docker, ejecutar SQL y verificar persistencia y conexión Keycloak/PostgreSQL; no existe realm importado.
- **Criterio de cierre:** contratos revisados y congelados por los responsables; PostgreSQL y Keycloak locales comprobados desde un arranque limpio. El Compose actual no incorpora aplicaciones.

### Fase 2 — Núcleo nativo

- **Completado:** requisitos y algoritmo base identificados en los documentos.
- **Pendiente:** contrato ABI, tamaños/límites, gestión de memoria, códigos de error, reglas de desempate por algoritmo, seis resultados manuales, biblioteca, algoritmos C, suite y compilación de `libplanificador.so`.
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

El contrato C/JNA todavía no existe: no se encontró cabecera C pública, documento ABI, estructura compilable ni adaptador Java. No iniciar esa integración hasta fijar orden y alineación de estructuras, tamaños máximos, propiedad/liberación de memoria y códigos de error.

## 8. Pruebas y benchmarks

- No se encontraron scripts de prueba, casos manuales versionados, pruebas C, pruebas JUnit ni pruebas de integración.
- No se encontraron datos de benchmark, resultados crudos, medianas ni informes de usabilidad.
- No se ejecutó ninguna prueba funcional o benchmark.
- La única validación de infraestructura observada es el parseo de configuración de Compose, que pasó sin iniciar contenedores.
- La sintaxis YAML de OpenAPI pudo parsearse con el parser disponible; sus referencias internas (19 esquemas y 6 respuestas) se resolvieron y los 9 `operationId` son únicos. Esto no equivale a validar el documento con un validador OpenAPI dedicado.
- El protocolo de Web 2 está especificado como 50, 500, 5.000 y 50.000 procesos, 30 repeticiones y mediana.
- El protocolo de Arquitectura está especificado como SRTF con 10.000 procesos, cinco ejecuciones consecutivas y descarte de tres por calentamiento JIT. No hay resultados. La cantidad de ejecuciones útiles después del descarte queda en dos, conforme a la decisión de equipo; los dos protocolos no deben mezclarse.

## 9. Problemas, riesgos y decisiones pendientes

1. **Reglas concretas de prioridad por algoritmo:** la regla general de tres niveles está confirmada; falta especificar dónde aplica dentro de FCFS/FIFO, empate de ráfaga en SJF, empate de tiempo restante en SRTF y orden/reingreso de Round Robin, sin convertirlos implícitamente en planificación por prioridad.
2. **Contrato C/JNA:** firmas ABI, alineación, límites, memoria y tabla de errores pendientes.
3. **OpenAPI:** endpoints, esquemas y códigos propuestos requieren aprobación; la comprobación realizada no fue una validación OpenAPI formal.
4. **Historial:** ubicación y propiedad por usuario están confirmadas; aún falta acordar campos exactos, retención, snapshots y semántica al pedir una corrida ajena.
5. **Stack:** siguen pendientes elecciones exactas dentro de los rangos acordados (Spring Boot, Angular, JNA y C11/C17).
6. **Documentación desactualizada:** `README.md` todavía dice que la discrepancia de dos/tres servicios está pendiente y lista versiones y decisiones ya resueltas como pendientes. `docs/arquitectura-add.md` también mantiene como pendientes el número de servicios, prioridad, origen de procesos, historial y orquestador. No se modificaron en esta tarea; corregirlos requiere una tarea documental separada.
7. **Infraestructura no ejecutada:** el Compose pasa validación estática, pero `docker compose ps` no pudo conectar con Docker Desktop (`dockerDesktopLinuxEngine` no disponible). No hay `infra/.env` local; SQL, Keycloak, persistencia y salud en ejecución no están verificados.
8. **Deriva Git:** el árbol tenía cambios locales y archivos no rastreados. Revisar antes de preparar cambios o commits; en particular, no eliminar la configuración `.idea/inspectionProfiles/` sin conocer su procedencia.

La decisión de exactamente dos microservicios está confirmada por el profesor y **no se reabre como tarea de arquitectura**. La regla de prioridad de tres niveles también está confirmada; solo resta especificar su aplicación por algoritmo.

## 10. Próxima tarea recomendada

**Acordar la forma y los datos de los casos de referencia antes de programar FIFO en C.**

Los documentos sí especifican seis categorías para los casos Web 2: llegadas simultáneas, empate de ráfaga, hueco de CPU ociosa, quantum mayor que la ráfaga más larga, un solo proceso y prioridades repetidas. También exigen resultados esperados calculados manualmente. No incluyen los valores concretos de entrada ni las salidas esperadas. Hay una diferencia de alcance que debe aclararse: el documento específico de Web 2 menciona que el docente entrega un conjunto desconocido de seis procesos y, además, pide verificar seis conjuntos de control (uno por categoría); el documento del núcleo nativo describe conjuntos de referencia de tres a cinco procesos. No asumir que esas frases describen el mismo conjunto ni fabricar valores para suplirlos.

- **Archivo/componente:** preparar la especificación de casos en `tests/` o en el documento de pruebas que acuerde el equipo; `tests/` está vacío. Antes de convertirla en datos de prueba, confirmar si se requieren seis conjuntos borde separados además de un conjunto de seis procesos para la sustentación, y acordar quién aporta o calcula manualmente sus entradas y resultados.
- **Resultado esperado:** alcance confirmado y oráculos con segmentos completos y métricas manuales. La regla general de prioridad permanece confirmada —menor número, luego menor llegada y finalmente orden original—; solo se debe precisar cómo se aplica en los puntos de decisión de cada algoritmo, especialmente en el caso de prioridades repetidas, sin cambiar sus políticas ni inventar una regla para el orden/reingreso de Round Robin.
- **Cómo comprobarlo:** validar el alcance con el equipo/docente, revisar manualmente los cálculos y confirmar que las categorías prescritas están cubiertas. Los oráculos deben quedar disponibles para las pruebas nativas y Java antes de implementar.
- **No modificar durante esta tarea:** documentos originales, decisiones confirmadas de exactamente dos microservicios y sin API Gateway, regla general de prioridad, contrato OpenAPI, Compose/SQL y código funcional. Después de cerrar los casos y los detalles pendientes del contrato C/JNA, la siguiente actividad de implementación será FIFO en C probado independientemente.

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
