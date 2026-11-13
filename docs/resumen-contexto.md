# Resumen de contexto y continuidad

**Estado vigente: 2026-11-13.** Este bloque sustituye las afirmaciones de estado actual del registro anterior, que se conserva debajo para no perder contexto ni trabajo local previo.

## Estado verificado

- Núcleo C con FCFS/FIFO, SJF, SRTF y Round Robin; no se alteraron sus implementaciones, desempates ni métricas en esta etapa.
- API común `planificador_ejecutar_v1()` implementada en `native/src/planificador.c` y declarada en `native/include/planificador.h`. Identificadores ASCII canónicos: `fcfs`, `sjf`, `srtf`, `rr`; sensibles a mayúsculas. Quantum positivo para RR, ignorado en los otros algoritmos.
- Cantidades/capacidades y estado `int32_t`; tiempos/quantum `int64_t`. Buffers NULL con capacidad cero permiten sondear. Estado 3 devuelve cantidades exactas sin escribir resultados. Métricas requeridas = procesos; segmentos incluyen ociosidad. Contadores obligatorios y distintos, buffers sin solapamiento, entrada estable entre llamadas.
- Estados 0–5 conservados. Ante error previo al sondeo los contadores quedan en cero. Si segmentos excede INT32_MAX: estado 5, segmentos cero, métricas igual a cantidad de procesos. Los errores posteriores al sondeo conservan sus tamaños. El límite del estado 5 no se ejecutó con miles de millones de segmentos; no se afirma cobertura integral de esa rama.
- `planificador_fcfs_v1()` retirado después de probar el reemplazo con el adaptador aún presente. La búsqueda en el repositorio no encontró consumidores ejecutables fuera de las pruebas; sus cuatro pruebas se adaptaron conservando comprobaciones importantes. No hay compatibilidad binaria con consumidores externos del símbolo retirado; no se identificaron consumidores de ese tipo en el proyecto.
- `native/src/fifo_didactico.c` movido íntegro a `native/referencias/fifo_didactico.c`. Autoría sin confirmar; se conserva como referencia académica y no se compila.
- Ubuntu/WSL 2: `make -C native clean`, `make -C native test` y `make -C native library` finalizaron correctamente el 2026-11-13. **34 casos pasaron**: 31 coberturas previas (cuatro adaptadas) y tres casos nuevos con múltiples escenarios de interfaz y ABI. Compilación C11 con advertencias estrictas, sin errores ni advertencias.
- `nm -D --defined-only native/libplanificador.so`: `planificador_fcfs`, `planificador_sjf`, `planificador_srtf`, `planificador_rr`, `planificador_ejecutar_v1`, `listar_algoritmos`. El símbolo antiguo ya no está exportado.
- ABI objetivo Linux x86-64: proceso, segmento y métricas de 32 bytes. Catálogo: `algoritmo_t` de 84 bytes, con id ASCII de 16, nombre de 64 y `int32_t requiere_quantum`. Tamaños y desplazamientos comprobados en C; Java verifica tamaños y la lectura real de arrays y tiempos de 64 bits.
- Integración Java 21 → JNA 5.18.1 → C verificada en Ubuntu/WSL. `PlanificadorBiblioteca` mapea estructuras/funciones y `PlanificadorNativo` carga una ruta absoluta configurable, sondea, reserva arrays contiguos, ejecuta y devuelve registros Java. La biblioteca se carga al consultar el catálogo o ejecutar una corrida.
- `listar_algoritmos()` devuelve memoria estática de solo lectura; Java copia identificadores, nombres y necesidad de quantum. El despachador busca identificadores en esta misma tabla C. No hay catálogo manual de producción en Java.
- Maven: **19 pruebas exitosas, sin fallos, errores ni omitidas** (nueve JNA, nueve HTTP y una de contexto Spring), con C real y H2/JWKS local de prueba. Incluyen ejecución, JWT, aislamiento por usuario, validaciones, CORS y quantum persistido/recuperado sin exponerlo en HTTP. Las 34 pruebas C también pasaron. Sin advertencias de compilación; el contexto conserva las advertencias de Mockito/Byte Buddy y agentes dinámicos de Java 21.
- PostgreSQL 16.4 y Keycloak 25.0.6 reales, con Docker Desktop, verificados: login, cuatro algoritmos, catálogo, POST 201 e historial privado. Se conservaron las mismas cuatro planificaciones después de reiniciar únicamente Java. Instrucciones y evidencia en `backend/planificador-service/docs/verificacion-real.md`.
- El snapshot `resultado` sigue siendo TEXT. El quantum se guarda solo cuando corresponde, se deserializa internamente y no aparece en la respuesta HTTP. Los registros antiguos sin quantum siguen siendo legibles con valor nulo; no se modificaron ni se migraron.
- Cierre de etapa: se recompiló y reinició únicamente la JVM en `localhost:8081`, aplicando el quantum. Con JWT de Keycloak real se creó una sola corrida RR con quantum 3, se recuperó su historial y SQL confirmó ese valor. Las huellas de los cuatro registros anteriores no cambiaron. PostgreSQL y Keycloak permanecieron activos. Evidencia y detalles en la guía de verificación real.
- Se usaron JDK Temurin 21.0.13+9 y Maven 3.9.11 Linux descargados temporalmente en `backend/planificador-service/target/herramientas/`, ignorado por Git. No se copió la biblioteca. Configuración: `-Dplanificador.biblioteca=/ruta/absoluta/native/libplanificador.so` o `PLANIFICADOR_BIBLIOTECA`. Instrucciones en el README del microservicio; las pruebas requieren la propiedad JVM para activarse.
- La ociosidad se identifica por `es_ocioso`, no por el PID; el núcleo devuelve actualmente PID 0 en intervalos ociosos. Se respetó esa salida sin modificar algoritmos.

## Arquitectura y contratos

Se mantienen exactamente dos microservicios (`proceso-service`, `planificador-service`), Angular directo sin API Gateway, PostgreSQL, Keycloak, Docker Compose local y Swarm como destino. Java 21/Spring Boot y JNA siguen siendo la ruta de integración. El motor Java equivalente de referencia también está pendiente. Angular presenta resultados y los promedios se calculan en backend. Historial propiedad del `sub` del JWT; cada simulación recibe la carga completa de procesos.

El proyecto backend implementado es `backend/planificador-service/`: REST, OAuth2 Resource Server JWT, validación, repositorio JDBC y JNA. PostgreSQL usa bases separadas: `planificador_db` para corridas, `proceso_db` para el servicio de procesos pendiente y `keycloak` para identidad. La única tabla de la aplicación es `public.planificaciones` (id, propietario, creado_en y resultado TEXT). No hay sistema local de usuarios ni claves foráneas a Keycloak; propietario contiene el sub validado.

`docs/openapi.yaml` permanece intacto como contrato compartido. Están implementados `GET /algoritmos`, `POST /planificaciones` (201), `GET /historial` y `GET /historial/{planificacionId}`. Todos requieren JWT; el historial ajeno devuelve 404. No se cambiaron rutas, campos ni formatos. `/comparaciones` y las operaciones de conjuntos no están implementados.

Para iniciar: configurar los `.env` locales ignorados por Git, levantar el Compose existente (override opcional limitado a la interfaz virtual de WSL NAT), exportar las variables y ejecutar Java 21 Linux en 8081. No copiar el `.so` ni borrar volúmenes. Los comandos se mantienen en el README del microservicio y su guía de verificación real para evitar duplicarlos aquí.

## Siguiente etapa

Continuar con `proceso-service`, integrar Angular sin alterar unilateralmente OpenAPI y preparar el despliegue final Swarm. Quedan también `/comparaciones`, motor Java de referencia, benchmarks, límites de tiempos representables exactamente en Angular y confirmación académica de los desempates provisionales. No se modifica el trabajo frontend de otra integrante.

Los benchmarks de Web 2 y Arquitectura mantienen sus metodologías separadas descritas en la bitácora. No se ejecutaron en esta etapa. El usuario autorizó cerrar, verificar y respaldar los avances en `feature/juanfer` de `origin`; el informe final indicará el commit y el resultado real del push.

---

## Registro anterior — instantánea al 2026-11-12

Lo siguiente se conserva como historial documental. Sus referencias al adaptador FCFS v1, 31 pruebas, símbolos antiguos y próximos pasos describen el estado anterior; para continuar usar el bloque vigente de arriba.

# Resumen de contexto y continuidad

**Proyecto:** Los Náufragos — Planificador de procesos PI  
**Última actualización del estado real:** 2026-11-12
**Propósito de este archivo:** resumir el estado verificable para que el equipo y un agente posterior puedan continuar sin confundir decisiones, propuestas documentales e implementación.

Este documento vivo no sustituye la bitácora. `docs/bitacora-decisiones.md` conserva las decisiones y sus razones; este resumen describe el estado actual observado, las tareas pendientes y la siguiente acción recomendada. Si ambos documentos difieren, verificar con el equipo y no reescribir silenciosamente el historial.

## Estado actual resumido — 2026-11-12

El núcleo nativo ya implementa **FCFS/FIFO, SJF, SRTF y Round Robin** en `native/src/planificador.c`. La cabecera pública `native/include/planificador.h` mantiene las cuatro interfaces, el adaptador `planificador_fcfs_v1()` y los códigos de error existentes. Las pruebas están separadas en `native/tests/test_planificador.c`.

El 2026-11-10 se simplificó el cálculo de métricas mediante `calcular_metricas()`, se optimizó Round Robin cuando el proceso puede terminar sin competencia y se añadieron dos pruebas para ráfagas grandes. La suite pasó con **31 casos**: se conservaron los 29 anteriores (incluidas las cuatro pruebas de FCFS v1) y se agregaron dos. `planificador.c` quedó en 1209 líneas frente a 1242 antes del cambio; la reducción fue principalmente por eliminar los helpers duplicados de métricas. `make -C native clean`, `make -C native test` y `make -C native library` se ejecutaron en WSL; la biblioteca exportó los cinco símbolos `planificador_fcfs`, `planificador_fcfs_v1`, `planificador_sjf`, `planificador_srtf` y `planificador_rr`. No hubo advertencias de compilación. Estos resultados corresponden a la ejecución del 2026-11-10; no se repitieron el 2026-11-12.

En palabras sencillas: el núcleo C ya puede calcular los cuatro algoritmos y devolver sus segmentos y métricas; las pruebas comprueban casos normales y errores. La biblioteca compartida también se genera. **Todavía no se ha conectado este núcleo con Java mediante JNA**, ni se han implementado los endpoints del servicio.

**Siguiente paso recomendado:** primero revisar y confirmar con el profesor los desempates provisionales de SJF y SRTF y el tratamiento de prioridad en los algoritmos que no son FCFS. Después, acordar y documentar el contrato C/JNA (estructuras, tipos, capacidades, errores y carga de la biblioteca) antes de escribir la integración Java. Con ese contrato aprobado, integrar y probar una llamada Java↔C aislada.

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
│   ├── src/planificador.c            # Implementación FCFS, SJF, SRTF y RR
│   ├── tests/test_planificador.c     # 31 casos nativos, incluidos ABI FCFS v1
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
| C con GCC, estándar C11 o C17 | `native/Makefile` solicita C11. Los cuatro algoritmos, ABI v1, 31 pruebas y el objetivo para la biblioteca compartida están implementados. Las pruebas y la biblioteca se verificaron en WSL el 2026-11-10. |
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
| Núcleo nativo C | **Implementado y probado localmente** | `planificador.c` contiene FCFS/FIFO, SJF, SRTF y RR; `planificador.h` mantiene las APIs y FCFS v1; `test_planificador.c` invoca 31 casos. `make clean`, `make test` y `make library` pasaron en WSL el 2026-11-10, y `nm -D` confirmó los cinco símbolos públicos. | Conservar las pruebas y fijar/aprobar el contrato C/JNA antes de integrar Java. Esto aún no representa aceptación de extremo a extremo ni aceptación oficial del profesor. |
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

- **Implementado:** FCFS/FIFO, SJF no expropiativo, SRTF expropiativo por eventos y Round Robin con quantum configurable en `native/src/planificador.c`.
- **API conservada:** `native/include/planificador.h` mantiene las cuatro funciones, `planificador_fcfs_v1()` y sus códigos de estado, incluido `PLANIFICADOR_CANTIDAD_NO_REPRESENTABLE = 5`.
- **Pruebas:** `native/tests/test_planificador.c` contiene 31 casos invocados, entre ellos las cuatro pruebas del adaptador FCFS v1 y dos casos nuevos de ráfagas grandes para RR.
- **Simplificación reciente:** los cuatro algoritmos reutilizan `calcular_metricas()`; RR salta turnos innecesarios cuando no existe competencia y el proceso actual acaba antes o justo al llegar el siguiente.
- **Verificado el 2026-11-10 en WSL:** limpieza, pruebas y generación de biblioteca pasaron; `nm -D` mostró los cinco símbolos esperados. La ejecución usó C11 y las opciones `-Wall -Wextra -Werror -pedantic -O2`, sin advertencias.
- **Pendiente:** confirmar con el profesor los criterios provisionales de SJF/SRTF y el uso de prioridad fuera de FCFS; acordar el contrato C/JNA y validar resultados contra oráculos oficiales. JNA no está implementado.
- **Criterio de cierre de integración:** además de las pruebas nativas, probar una llamada Java↔C con mapeo de estructuras, capacidades y errores aprobado.

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

- `native/tests/test_planificador.c` invoca **31 casos nativos** para FCFS/FIFO, SJF, SRTF y RR. Incluye CPU ociosa, métricas, desempates, expropiación, quantum, capacidad insuficiente, argumentos inválidos, desbordamientos y cuatro casos del adaptador FCFS v1. Los dos casos añadidos el 2026-11-10 verifican ráfagas de mil millones con quantum 1: una sin competencia y otra que termina antes de la siguiente llegada.
- Verificación nativa más reciente registrada: **2026-11-10 en WSL**. Pasaron `make -C native clean`, `make -C native test` y `make -C native library`; compilación C11 con `-Wall -Wextra -Werror -pedantic -O2`, sin advertencias. `nm -D --defined-only native/libplanificador.so` mostró los cinco símbolos públicos de los algoritmos y FCFS v1. No se repitieron estas comprobaciones el 2026-11-12.
- Las expectativas de la suite nativa son casos de desarrollo; no se registran como oráculos oficiales del profesor. No hay integración Java↔C ni pruebas de aceptación de extremo a extremo.
- La prueba básica de carga de contexto de `planificador-service` se ejecutó el 2026-10-22 y pasó: 1 prueba, 0 fallos, 0 errores, 0 omitidas. Este resultado es anterior y no verifica JNA ni endpoints funcionales.
- No se encontraron scripts de benchmark, datos, resultados crudos, medianas ni informes de usabilidad.
- La única validación de infraestructura observada es el parseo de configuración de Compose, que pasó sin iniciar contenedores.
- La sintaxis YAML de OpenAPI pudo parsearse con el parser disponible; sus referencias internas (19 esquemas y 6 respuestas) se resolvieron y los 9 `operationId` son únicos. Esto no equivale a validar el documento con un validador OpenAPI dedicado.
- El protocolo de Web 2 está especificado como 50, 500, 5.000 y 50.000 procesos, 30 repeticiones y mediana.
- El protocolo de Arquitectura está especificado como SRTF con 10.000 procesos, cinco ejecuciones consecutivas y descarte de tres por calentamiento JIT. No hay resultados. La cantidad de ejecuciones útiles después del descarte queda en dos, conforme a la decisión de equipo; los dos protocolos no deben mezclarse.

## 9. Problemas, riesgos y decisiones pendientes

1. **Reglas concretas de prioridad por algoritmo:** FCFS/FIFO conserva llegada primero; con igual llegada, menor prioridad numérica y luego orden original. SJF y SRTF usan criterios provisionales documentados en código y cabecera, pendientes de confirmación del profesor. RR ordena llegadas simultáneas por posición original y la prioridad no altera la cola FIFO; confirmar que estas reglas corresponden a lo solicitado en clase.
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

**Acordar los criterios de desempate pendientes y aprobar el contrato C/JNA antes de integrar Java.**

Los cuatro algoritmos ya están implementados y probados de forma nativa, y `libplanificador.so` se generó correctamente en WSL. La API nativa actual conserva FCFS v1, pero la compatibilidad de símbolos no significa que el mapeo JNA esté aprobado ni probado.

- **Primero:** confirmar con el profesor los desempates provisionales de SJF/SRTF y el papel de la prioridad en SJF, SRTF y RR.
- **Después:** revisar `native/include/planificador.h` y acordar tipos y alineación de estructuras, manejo de cantidades/capacidades, códigos de error, buffers y plataforma de carga de `libplanificador.so`.
- **Resultado esperado:** un contrato C/JNA aprobado que permita mapear las estructuras sin ambigüedad y traducir errores correctamente, sin cambiar las firmas públicas de forma unilateral.
- **Verificación posterior:** integrar JNA en `planificador-service`, probar una llamada aislada de Java a C y volver a ejecutar `make -C native test`.
- **Fuera de alcance de esa etapa:** implementar endpoints, frontend, persistencia o catálogo dinámico; no tocar la lógica de los algoritmos sin una etapa autorizada.

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

Este resumen organiza los ocho requisitos funcionales proporcionados por el equipo. No se confirmó acceso directo a Jira ni se verificó el estado de sus tickets. RF-03 está **En progreso**: los cuatro algoritmos y la biblioteca compartida están implementados y probados en C; faltan JNA, `POST /planificaciones` y la aceptación de extremo a extremo. Los otros siete requisitos están **Pendiente**.

| ID / requisito | Objetivo | Componentes implicados | Criterio principal de aceptación | Estado |
|---|---|---|---|---|
| RF-01 — Registrar y persistir procesos/conjuntos | Registrar procesos con identificadores únicos dentro del conjunto y persistirlos. | `frontend/`, `proceso-service`, PostgreSQL, Compose. | Recuperar un conjunto de cinco procesos después de reiniciar el contenedor y comprobar que los datos se conservan. | **Pendiente** |
| RF-02 — Guardar y recuperar conjuntos de prueba con nombre | Guardar un conjunto nombrado y recuperarlo usando los endpoints de conjuntos correspondientes. | `frontend/`, API de `proceso-service`, PostgreSQL. | Guardar y volver a cargar el mismo conjunto por su nombre mediante los endpoints acordados. | **Pendiente** |
| RF-03 — Ejecutar FIFO en C mediante JNA | Ejecutar `POST /planificaciones` con FIFO en C integrado desde Java mediante JNA. | `planificador-service`, `libplanificador.so`, JNA, validación del JWT. | Los tiempos de los segmentos devueltos coinciden exactamente con el diagrama de Gantt de referencia. Los cuatro algoritmos están probados localmente en C; HTTP/JNA no están implementados. | **En progreso** |
| RF-04 — Quantum configurable para Round Robin | Permitir cambiar el quantum en cada corrida sin compilar de nuevo. | `frontend/`, `planificador-service`, API, núcleo C/JNA. | Ejecutar la misma entrada con quantum 2 y 5 y comparar resultados sin recompilar. | **Pendiente** |
| RF-05 — Diagrama de Gantt con ociosidad | Mostrar segmentos de ejecución y periodos de CPU ociosa. | `frontend/`, respuesta de `planificador-service`. | Visualizar ocho procesos a 1366 px sin desplazamiento horizontal, distinguiendo cada segmento y los periodos ociosos. | **Pendiente** |
| RF-06 — Métricas por proceso y promedios | Calcular espera, retorno y respuesta por proceso y sus promedios en el backend. | `planificador-service`, núcleo/motor de planificación, `frontend/`. | Verificar los valores y promedios con resultados manuales; Angular presenta, no calcula los promedios. | **Pendiente** |
| RF-07 — Comparar algoritmos sobre la misma entrada | Ejecutar todos los algoritmos con la misma entrada y comparar sus métricas promedio. | `planificador-service`, catálogo dinámico, núcleo C/JNA o motor Java, `frontend/`. | Mostrar la comparación común e identificar el algoritmo con mejor métrica promedio según el criterio aprobado. | **Pendiente** |
| RF-08 — Validar entradas con errores HTTP 400 JSON | Validar ráfaga > 0, llegada ≥ 0 y quantum ≥ 1. | Validación en Angular y autoritativa en `planificador-service`/API. | Cada dato inválido responde HTTP 400 con JSON de error; el formato exacto de ese JSON debe quedar en el contrato aprobado. | **Pendiente** |

**Base de los criterios:** *Los Náufragos – Planificador de procesos*, PDF, pp. 5–6, RF-01 a RF-08. Los componentes se asignan según las responsabilidades arquitectónicas confirmadas en este resumen. CPU ociosa se devuelve como parte de la línea de tiempo; no se añade un servicio para ello.

### Secuencia recomendada para implementar el backlog

**Estado de las condiciones previas:** los cuatro algoritmos nativos y sus pruebas están implementados; el contrato C/JNA, la confirmación de los desempates provisionales, los oráculos oficiales y el contrato HTTP siguen pendientes. Los resultados de desarrollo no se atribuyen al profesor ni se consideran aceptación de RF-03.

1. **Desempates y contrato C/JNA.** Confirmar con el profesor las reglas provisionales y aprobar tipos, alineación, capacidades, errores y carga de la biblioteca antes de codificar la interfaz Java.
2. **Integración Java/JNA.** Mapear el contrato aprobado en una interfaz aislada y validar una llamada C→JNA; implementar también el motor Java de referencia y comprobar equivalencia funcional.
3. **`planificador-service`.** Implementar el adaptador de motor, validación/backend y las operaciones de planificación y catálogo según el OpenAPI revisado; calcular promedios en backend y persistir historial asociado al `sub`.
4. **`proceso-service` y persistencia de conjuntos.** Administrar procesos y conjuntos, validar unicidad y entradas, y verificar persistencia tras reinicio. Una corrida individual sigue recibiendo la carga completa de Angular y no depende de que esté guardada.
5. **Seguridad e integración de interfaz.** Configurar Keycloak/JWT y permisos en ambos servicios; conectar Angular para capturar/cargar datos, seleccionar algoritmos desde el catálogo dinámico, ejecutar/comparar, presentar Gantt y métricas, y comunicar validaciones.
6. **Aceptación funcional y evidencias.** Ejecutar los ocho criterios del backlog, casos manuales y pruebas de integración. Mantener separados los dos protocolos de benchmark. Registrar resultados únicamente después de ejecutarlos.

La secuencia mantiene exactamente `proceso-service` y `planificador-service`, sin API Gateway. El orden C→JNA→servicio→interfaz sigue la guía maestra. La política confirmada para FCFS/FIFO se aplica únicamente a este algoritmo; no fija por sí sola los desempates de SJF, SRTF ni Round Robin.

## Estado actual y siguiente paso

**Actualización: 2026-11-12.** Los resultados de compilación y pruebas nativas que siguen corresponden a la ejecución verificada el 2026-11-10.

### Arquitectura acordada

- Frontend Angular.
- Exactamente dos microservicios Spring Boot: `proceso-service` y `planificador-service`.
- Biblioteca nativa en C llamada desde Java mediante JNA.
- Keycloak para autenticación.
- No introducir un API Gateway.

### Trabajo nativo realizado

- `native/src/planificador.c` implementa FCFS/FIFO, SJF, SRTF y RR. El cálculo de métricas se comparte mediante `calcular_metricas()`; el helper toma primer inicio y finalización, por lo que también funciona para los algoritmos expropiativos.
- FCFS conserva su política de desempate. SJF y SRTF mantienen sus reglas provisionales; RR conserva la cola FIFO y agrega al proceso que llega antes de reinsertar el que agotó quantum.
- Round Robin evita simular turnos redundantes si el proceso actual puede finalizar antes o exactamente cuando llegue el siguiente proceso y no hay otro listo.
- `native/include/planificador.h` no se cambió en la simplificación: conserva las APIs, `planificador_fcfs_v1()` y `PLANIFICADOR_CANTIDAD_NO_REPRESENTABLE = 5`.
- `native/tests/test_planificador.c` conserva los 29 casos anteriores y añade dos pruebas de ráfagas grandes, para 31 casos. Se mantienen las cuatro pruebas del adaptador FCFS v1.
- El 2026-11-10, `make -C native clean`, `make -C native test` y `make -C native library` finalizaron correctamente en WSL. La compilación no mostró advertencias. `nm -D --defined-only native/libplanificador.so` confirmó `planificador_fcfs`, `planificador_fcfs_v1`, `planificador_sjf`, `planificador_srtf` y `planificador_rr`.
- `planificador.c` pasó de 1242 a 1209 líneas; la reducción provino de quitar dos helpers duplicados de métricas y simplificar comentarios, no de eliminar algoritmos ni pruebas.

### Situación de la integración Java

- `backend/planificador-service` contiene `pom.xml`, `PlanificadorServiceApplication.java`, `PlanificadorServiceApplicationTests.java` y `README.md`. El POM declara Java 21 y Spring Boot 3.5.7.
- El 2026-10-22, Maven 3.9.16 incluido con IntelliJ ejecutó `mvn -f backend/planificador-service/pom.xml test`: BUILD SUCCESS, 1 prueba, 0 fallos, 0 errores y 0 omitidas. Las advertencias de Mockito/agentes dinámicos no impidieron el éxito.
- La configuración Spring Boot de IntelliJ inició la aplicación; se verificó el puerto 8080 escuchando y una respuesta HTTP 404 en `/`, esperable porque aún no hay ruta raíz. Se detuvo la sesión después de comprobarlo. Un intento separado de `mvn spring-boot:run` falló al descargar dependencias del plugin por resolución DNS de Maven Central.
- No hay endpoints funcionales de planificación, algoritmos Java ni integración JNA. JNA sigue pendiente.
- La biblioteca disponible es Linux x86-64. Antes de cargarla desde Java hay que confirmar el entorno de ejecución y usar un entorno/plataforma compatible.

### Siguiente paso acordado

1. Consultar al profesor y confirmar los criterios provisionales de empate de SJF/SRTF y el rol de prioridad en SJF, SRTF y RR.
2. Acordar el contrato C/JNA: disposición de estructuras, anchos de tipos, tamaños/capacidades, errores y plataforma/carga de la biblioteca.
3. Solo después de aprobarlo, implementar el mapeo JNA en `planificador-service` y verificar una llamada Java↔C aislada.
4. Continuar con endpoints y catálogo dinámico según el OpenAPI revisado; luego con persistencia, seguridad y frontend.

El núcleo C tiene una base nativa probada, pero **la integración JNA todavía no existe**. La compilación de la biblioteca y la exportación de símbolos no prueban por sí solas que una llamada desde Java funcione.

### Archivos del esqueleto inicial del microservicio

- `backend/planificador-service/pom.xml`: configuración Maven, Java 21, Spring Boot 3.5.7, Spring Web y pruebas.
- `backend/planificador-service/src/main/java/com/losnaufragos/planificador/PlanificadorServiceApplication.java`: punto de entrada Spring Boot.
- `backend/planificador-service/src/test/java/com/losnaufragos/planificador/PlanificadorServiceApplicationTests.java`: prueba básica de carga del contexto.
- `backend/planificador-service/README.md`: requisitos y comandos de prueba/ejecución.

### Estado Git al actualizar este resumen

- Rama observada: `feature/juanfer`.
- Antes de modificar este documento, estaban modificados `native/src/planificador.c` y `native/tests/test_planificador.c`, por los cambios aprobados de simplificación y pruebas. También quedará modificado este resumen.
- No se modificaron la cabecera, Java/JNA, microservicios ni otros documentos. No se creó ningún commit.
