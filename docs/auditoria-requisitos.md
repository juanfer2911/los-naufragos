# Auditoría de requisitos — 2026-11-16

Rama auditada: `integracion`, base `c63536a`, incluidos los cambios locales de integración Angular–proceso-service, roles y quantum. Esta auditoría no incorpora funcionalidades ni refactorizaciones. El usuario confirmó además la comprobación manual de los flujos principales.

## Fuentes y criterio

Se leyeron completos los dos documentos encontrados, mediante extracción de texto de todas sus páginas:

- **P:** [`- Los Naufragos - Planificador de procesos.pdf`](<- Los Naufragos - Planificador de procesos.pdf>), 22 páginas.
- **N:** [`Los Naufragos - Nucleo de planificacion.pdf`](<Los Naufragos - Nucleo de planificacion.pdf>), 11 páginas.

Las referencias P/N indican página del PDF, contando desde la portada. No se sustituyeron estos documentos por README ni por la guía Copilot. El enunciado general y la guía maestra están presentes, pero no son fuentes adicionales de requisitos de esta auditoría solicitada sobre los dos documentos anteriores.

**Cumplido:** implementación y pruebas sustentan la obligación descrita. **Parcial:** existe parte, pero falta comportamiento o alguna evidencia exigida. **Pendiente:** no se encontró implementación/entregable. **Por verificar:** no puede concluirse mediante código y pruebas disponibles, por ejemplo una demostración ante el docente. No se equipara una prueba local con una aceptación formal o con un despliegue limpio.

**Alcance temporal:** esta auditoría es la fotografía de la base `c63536a`; no incorpora el trabajo posterior de Swarm completo, motor Java, bancos de medición en contenedor ni el ajuste de vida del token. Para el estado vigente de esos frentes, consulta [ADD](arquitectura-add.md), [resumen de continuidad](resumen-contexto.md) y [evidencias de seguridad/usabilidad](evidencias-seguridad-usabilidad.md). Las observaciones de requisito que dependen del docente o de participantes siguen abiertas hasta obtener evidencia.

Abreviaturas de evidencia (rutas relativas a la raíz del repositorio):

- **UI:** `frontend/src/app/app.ts` y `app.html`; **AUTH:** `frontend/src/app/auth.service.ts`.
- **PLAN:** `backend/planificador-service/src/main/java/com/losnaufragos/planificador/`; **PROC:** `backend/proceso-service/src/main/java/com/losnaufragos/proceso/`.
- **JNA:** `PLAN/nativo/PlanificadorNativo.java` y `PlanificadorBiblioteca.java` (sustituir PLAN por la ruta anterior).
- **CT:** `native/tests/test_planificador.c`; **JT:** `backend/planificador-service/src/test/java/com/losnaufragos/planificador/nativo/PlanificadorNativoTests.java`.
- **PT:** `backend/planificador-service/src/test/java/com/losnaufragos/planificador/ApiIntegracionTests.java`; **ST:** `backend/proceso-service/src/test/java/com/losnaufragos/proceso/ApiIntegracionTests.java`.

## Requisitos funcionales obligatorios

| Requisito del documento | Estado | Evidencia concreta | Qué falta |
|---|---|---|---|
| P RF-01, p.5: procesos con identificador, llegada, ráfaga, prioridad y PID único; cinco procesos sobreviven al reinicio de contenedores | Parcial | UI `guardarProceso`; PROC `ApiModelos.Proceso`, `ConjuntoServicio.crear`; PK conjunto/PID en `schema.sql`; ST rollback/validación | Persistencia implementada y ya comprobada tras recarga/reinicio Java. Falta registrar el ensayo exacto de cinco procesos y reinicio de contenedores exigido por el PDF. No se reiniciaron contenedores en esta auditoría. |
| P RF-02, p.5: guardar y recuperar cargas con nombre, sin recapturar números | Cumplido | `frontend/src/app/proceso-api.service.ts`; UI `guardarConjunto`, `cargarConjunto`; PROC GET/POST y tablas `conjuntos`/`procesos`; pruebas Angular de carga | — |
| P RF-03, p.5: elegir algoritmo, obtener segmentos/métricas y coincidir con el conjunto manual oficial FIFO | Parcial | UI `ejecutarSimulacion`; PLAN `PlanificacionServicio.ejecutar`; CT/JT comparan segmentos esperados | Función operativa; no se encontró el conjunto oficial fotografiado/resuelto a mano y su aceptación con el profesor. |
| P RF-04, p.5: quantum por corrida editable; misma carga con 2 y 5 cambia segmentos/promedios sin recompilar | Parcial | UI `quantum` y campo editable; JT `quantumSeTransmiteAC`; CT `probar_rr_basico_y_metricas`; pruebas Angular de 8 guardado y 5 ejecutado | Falta evidencia automatizada/específica del par 2/5 y sus promedios sobre la misma carga. La capacidad está implementada y comprobada con otros valores. |
| P RF-05, p.6: Gantt con marcas temporales, proceso y ocio; ocho procesos legibles a 1366 px sin desplazamiento horizontal | Parcial | UI `rebanadasGantt`; `app.html` distingue CPU ociosa, usa barras flexibles y marcas | No se ejecutó la aceptación visual exacta de ocho procesos/1366 px. `overflow:hidden` y `min-width:0` pueden recortar etiquetas en muchas rebanadas; requiere verificar legibilidad. |
| P RF-06, p.6: espera, retorno y respuesta por proceso y promedios calculados en backend | Cumplido | C `calcular_metricas`; PLAN `PlanificacionServicio.ejecutar` calcula promedios; JT verifica enteros; UI solo representa los resultados | La aceptación contra el control oficial se registra aparte en RF-03/RQ-02. |
| P RF-07, p.6: una acción ejecuta todos y muestra los cuatro promedios de las tres métricas y ganador por cada una | Parcial | UI ejecuta los algoritmos seleccionados con un botón; `resultados` conserva todos; `algoritmoGanador` solo evalúa espera | La pantalla muestra resultados por pestañas, no tabla conjunta de 4×3 métricas; faltan ganadores de retorno/respuesta y tratamiento explícito de empates. `/comparaciones` existe solo en OpenAPI; no es obligatorio ese endpoint concreto si la experiencia cumple. |
| P RF-08, p.6: rechazar entradas inválidas indicando campo y valor esperado | Parcial | `@Min`, `@NotNull`, unicidad y pruebas 400 en ambos servicios; UI valida antes de ejecutar | UI `guardarProceso` puede retornar silenciosamente ante ráfaga 0/llegada negativa. `ErroresApi.entrada` emite mensaje genérico en errores de Bean Validation; falta detalle útil por campo. |
| P p.2 y RQ-02 p.9: prioridades y desempates documentados y visibles | Parcial | `native/include/planificador.h` documenta FCFS, SJF, SRTF y RR; CT prueba empates | Reglas SJF/SRTF siguen marcadas provisionales; no se muestran en la interfaz. No se infiere de aquí un quinto algoritmo de prioridad obligatorio. |

## Identidad y autorización obligatorias (Web 2)

| Requisito del documento | Estado | Evidencia concreta | Qué falta |
|---|---|---|---|
| P RA-01, p.12: Keycloak, realm/roles/clientes/usuarios de cada rol importados automáticamente al levantar todo | Parcial | `infra/docker-compose.yml`; `backend/planificador-service/dev/keycloak-realm.json` define cliente y roles | Compose no configura importación del realm; plantilla sin usuarios de prueba. Falta aprovisionamiento seguro y automático para un despliegue limpio, sin publicar credenciales reales. |
| P RA-02, p.12: cliente público, Authorization Code+PKCE, pantalla Keycloak; retorno a ruta protegida | Parcial | AUTH `inicializar`/`iniciarSesion`: `flow:standard`, `pkceMethod:S256`; plantilla pública sin secret | El flujo funciona. `app.routes.ts` está vacío, el acceso se inicia con botón y `redirectUri` fija `/`; no hay redirección automática ni retorno a una vista/ruta protegida solicitada como pide el criterio literal. |
| P RA-03, p.12: interceptor Bearer y backend como autoridad de acceso | Cumplido | AUTH `jwtInterceptor` restringido a URLs de ambos backends; pruebas de ambos clientes HTTP y de no filtrar a otros orígenes; ST 403 | — |
| P RA-04, p.12: cada servicio valida firma, exp, issuer, audiencia; 401 sin token y firma alterada | Parcial | Ambas clases `Seguridad.jwtDecoder`; PT prueba firma alterada, issuer, audiencia y expiración; ST prueba JWT inválido/issuer/audiencia/expiración | Implementación presente. Falta caso específico de JWT firmado y después alterado en ST y un registro reproducible equivalente contra los dos servicios con Keycloak real. |
| P RA-05, p.13–14: docente administra; estudiante ejecuta/consulta y recibe 403 en POST y DELETE conjuntos | Cumplido | PROC `Seguridad.filtroSeguridad`, `autenticacion` lee `realm_access.roles`; ST `cicloCompartidoYRoles`, `quantumOpcionalSePersisteYValida`; UI oculta controles | — |
| P RA-06, p.13: persistir identidad `sub`, algoritmo y entrada de cada corrida; comprobar dos usuarios | Cumplido | PLAN controlador usa JWT; `PlanificacionRepositorio.guardar/listar/buscar` filtra por propietario; PT historial privado de distintos sujetos | El historial contiene snapshot de procesos y `sourceSetId` opcional; no depende de que el conjunto siga existiendo. |
| P RA-07, p.13: vida corta declarada, renovación/logout real; demostrar token de 1 minuto y botón Atrás | Parcial | AUTH `updateToken(30)`, `onTokenExpired`, manejo de error y `keycloak.logout`; verificaciones previas de login/logout | Plantilla no declara `accessTokenLifespan`; no se encontró ensayo registrado de expiración a 60 s y navegación atrás posterior al logout. |
| P p.14–15/20: justificar lugar de autorización y lectura del claim; registro reproducible de 401/401/403/éxito | Parcial | `docs/bitacora-decisiones.md`, dos clases Seguridad, PT/ST y guías HTTP | Completar evidencia versionable y sin tokens reales de todos los casos exactos en cada servicio. Las respuestas exitosas de POST/DELETE son 201/204 según OpenAPI, no forzar 200. |

## Arquitectura y restricciones técnicas obligatorias

| Requisito del documento | Estado | Evidencia concreta | Qué falta |
|---|---|---|---|
| P p.15: Angular standalone y signals; ningún algoritmo ni promedio calculado en navegador | Cumplido | UI `@Component(standalone:true)`, signals/computed; clientes HTTP; promedios vienen de `Planificacion` | Ordenar resultados para mostrar ganador no simula planificación ni recalcula promedios. |
| P p.15–16: exactamente dos microservicios Spring Boot y separación real | Cumplido | Ambos POM y controladores; PROC solo guarda entradas; PLAN llama C y guarda su historial | — |
| P p.15: base propia para proceso-service | Cumplido | `infra/postgres/init/01-create-databases.sql`, `proceso_db`/`proceso_user`; aplicaciones con URLs distintas | — |
| P p.16: justificar gateway y transporte de procesos entre servicios | Cumplido | Bitácora D-16 y decisiones de carga completa; UI envía lista completa y sourceSetId a PLAN; no llamadas PLAN→PROC | La propuesta del PDF admite expresamente ambas opciones; no exige llamada entre servicios ni API Gateway. |
| P p.15–17/N p.1–2: algoritmos en C, biblioteca Linux invocada por JNA | Parcial | `native/src/planificador.c`, Makefile `.so`, JNA `Native.load`, pruebas Linux reales | Integración local cumplida; falta biblioteca dentro de imagen de producción. |
| P p.18/N p.5/9: adaptador/servicio encapsula JNA, mapea estructuras y traduce errores | Cumplido | JNA `PlanificadorBiblioteca`; `PlanificadorNativo`; PLAN `PlanificacionServicio` adapta a DTO; controlador sin tipos JNA | — |
| P p.4/17/N p.4–5: contrato binario, propiedad de memoria y códigos escritos | Parcial | `native/include/planificador.h`, tabla 0–5, buffers de Java, catálogo estático C; `comprobarEstado`; pruebas ABI | Contrato actual v1 difiere del ejemplo literal del PDF (tipos de 64 bits, ocio por flag, catálogo estructurado y códigos positivos). Cambios documentados internamente, pero su aceptación por el docente no está demostrada. |
| P p.17: una llamada nativa por corrida, resultado completo y métricas agregadas | Parcial | JNA `ejecutar` hace sondeo y ejecución en dos llamadas a `planificador_ejecutar_v1`; C no calcula promedios | No hay llamada por rebanada, pero son dos cruces y el wrapper vuelve a sondear internamente. Aclarar/medir o adecuar frente al texto literal, sin cambiarlo en esta tarea. |
| P p.4/N p.2: motor Java equivalente y selección configurable como respaldo sin `.so` | Pendiente | PLAN solo dispone de `PlanificadorNativo`; `ConfiguracionNativa` exige ruta de biblioteca; búsqueda de fuentes sin otro motor | Implementación Java de referencia y selección por configuración. No confundir JNA/DTO en Java con un planificador Java. |
| P p.20: contrato OpenAPI versionado y catálogo abierto | Parcial | `docs/openapi.yaml`, identificador string libre; GET `/algoritmos` deriva del catálogo C; quantum de conjunto opcional | El contrato aún se describe como borrador y anuncia `/comparaciones` no implementado. No cambiar rutas por la auditoría; reconciliar documentación/alcance aprobado. |
| P p.15: historial con aportes de todos los integrantes | Cumplido | `git log --all --format=%an` muestra `juanfer2911` y `mila-1910` (también `root`) | No reescribir autoría. La comprensión compartida se verifica por separado. |

## Núcleo de planificación: funcionalidad y calidad

| Requisito del documento | Estado | Evidencia concreta | Qué falta |
|---|---|---|---|
| N RN-01, p.2: FCFS sin desalojo y ocio explícito | Cumplido | C `planificador_fcfs`; CT `probar_llegadas_desempates_y_ocio`, `probar_un_proceso_y_ocio_inicial`; JT cuatro algoritmos | — |
| N RN-02, p.3: SJF menor ráfaga entre llegados, no expropiativo | Cumplido | C `planificador_sjf`; CT `probar_sjf_llegadas_ocio_no_expropiacion_y_metricas`; JT `sjfSeleccionaLaRafagaMenor` | Aprobación del oráculo oficial aparte. |
| N RN-03, p.3: SRTF expropiativo, ejemplo de un proceso con tres rebanadas | Parcial | C `planificador_srtf`; CT `probar_srtf_expropiacion_y_metricas` compara la secuencia y métricas, con dos rebanadas de P1 | Expropiación implementada; falta el caso de aceptación explícito de un mismo proceso en tres rebanadas separadas. |
| N RN-04, p.3: RR circular, quantum parámetro, q=2 y q grande equivalente a FCFS | Parcial | C `planificador_rr`; CT RR básico y orden de reingreso; JT `quantumSeTransmiteAC` verifica equivalencia con q=20 | La equivalencia se prueba con una carga concreta. En llegadas empatadas FCFS usa prioridad y RR orden original: la afirmación general depende de la regla de desempate acordada. |
| N RN-05, p.3: métricas exactas y promedios | Parcial | C `calcular_metricas` y CT/JT comparan enteros; PLAN calcula los tres promedios | El sistema devuelve promedios, pero el alcance del núcleo pide también producirlos desde C; ABI actual no los incluye. Aclarar reparto entre documentos/frentes. |
| N RN-06, p.3: catálogo nativo y quinto algoritmo sin tocar Java | Parcial | C `listar_algoritmos`, `catalogo`, despacho; JNA catálogo; Angular genera selector desde API | Falta demostración y diff de quinto algoritmo. JT `catalogoProcedeDeC` espera los cuatro IDs exactos: ese test necesitaría revisarse en el ensayo de ampliación, aunque el runtime sea dinámico. |
| N RQ-01, p.6: SRTF 10.000 procesos C/JNA vs Java, igualdad y corrida <1 s | Pendiente | No hay motor Java, generador de carga ni banco de medición en las fuentes | Semilla fija, datos crudos, límites de contenedor y comparación completa. El PDF pide cinco corridas/mediana, pero también descartar tres por JIT: aclarar esa ambigüedad antes de medir. |
| N RQ-02, p.7: un comando, ≥2 cargas por algoritmo, segmentos completos, <30 s, clon limpio contra biblioteca | Parcial | CT tiene 34 casos y aserciones de segmentos/métricas, `make -C native test` pasa; 0,1195 s medidos en este equipo para ejecución incremental | Makefile enlaza directamente el fuente C en el ejecutable de pruebas, no el `.so`. JT sí usa el `.so`, pero requiere Java. Falta demostrar protocolo completo en clon limpio y reporte por caso esperado/obtenido. |
| N RQ-03, p.7–8: imagen <250 MB sin gcc; primer resultado <90 s; tres construcciones sin caché/mediana | Pendiente | No hay Dockerfile ni imagen propia ni stack completo | Dockerfile multietapa, mediciones crudas de tamaño/arranque y aceptación en máquina limpia. |

## Calidad Web 2 y entregables obligatorios

| Requisito del documento | Estado | Evidencia concreta | Qué falta |
|---|---|---|---|
| P RQ-01, p.7–8: quinto algoritmo en vivo ≤30 min, ≤2 módulos C, cero Java/Angular/OpenAPI | Por verificar | Runtime genérico, catálogo C y string abierto en API; UI `algoritmosDisponibles` | Ensayo en frío ante docente, commits antes/después, diff --stat y diff vacío de OpenAPI; no implementar un quinto ahora ni preparar una rama oculta. |
| P RQ-02, p.8–9: seis controles manuales, JUnit en cada construcción, cero diferencias en segmentos/promedios a dos decimales | Parcial | CT cubre casos borde y JT tiene resultados esperados literales; PT ejecuta los cuatro algoritmos | No existe la matriz canónica 6×4 con todas las métricas/promedios ni fotografía del control de seis procesos. Pruebas JNA se omiten sin propiedad de biblioteca; no hay pipeline de construcción que garantice ejecutarlas. |
| P RQ-03, p.9–10: tres personas ajenas, cinco procesos, ≤12 interacciones, ≤5 min, cero preguntas | Pendiente | UI utilizable y pruebas de componente/navegador previas | Sesiones con personas reales, cronómetro, conteo y registro literal de preguntas; no sustituir por pruebas automatizadas. |
| P p.18: benchmark de los cuatro algoritmos, 50/500/5.000/50.000 procesos, 30 repeticiones y mediana | Pendiente | No se encontraron generadores ni benchmark ni motor Java | Banco independiente del de Arquitectura; igualdad de resultados, mediana y punto de cruce. |
| P p.3–4/18 y N p.8–9: Dockerfile multietapa, tamaño/arranque con y sin `.so` | Pendiente | Solo Makefile y JAR local; no Dockerfile en repositorio | Compilar con gcc en etapa de construcción, copiar biblioteca/JAR a runtime sin gcc ni fuentes, datos antes/después. |
| P p.3/15/19 y N p.8–9: stack Swarm o Kubernetes completo con límites CPU/memoria; un comando desde máquina limpia | Pendiente | `infra/docker-compose.yml` levanta solo PostgreSQL/Keycloak; override WSL solo redes | Angular y servicios no están en manifiesto, faltan límites e importación automática. Swarm está elegido en bitácora; Compose local no satisface esta entrega. |
| N p.8: ruta de carga `.so` dentro de contenedor, comportamiento si falta y decisión en bitácora | Parcial | JNA carga ruta absoluta y produce error si falta; variable `PLANIFICADOR_BIBLIOTECA` | Ruta/ensayo dentro de imagen final y respaldo Java; no existe esa imagen. |
| P p.19: repositorio y README para arrancar desde cero | Parcial | Fuentes presentes, instrucciones IntelliJ/WSL/PowerShell | Instrucciones dependen de herramientas locales temporales y varios arranques/manuales; falta un comando para máquina limpia. |
| P p.19 y N p.10: ADD de cinco pasos y decisiones con fecha, alternativas, motivo, costo y requisito de calidad | Parcial | `docs/arquitectura-add.md` contiene estructura de diseño; `docs/bitacora-decisiones.md` decisiones D-01–D-22 y notas | ADD conserva estados obsoletos (directorios vacíos, contratos por decidir, motor Java como diseñado). Reconciliar con implementación y uniformar fechas/alternativas/costos/atributo en cada decisión. |
| P p.19: cuatro anexos de medición; N p.9: tres mediciones con datos crudos | Pendiente | Hay evidencia funcional local y algunos JSON históricos de integración, no anexos de calidad completos | Diffs quinto algoritmo, tabla de controles, usabilidad, benchmarks y tamaño/arranque; separar protocolos por curso. |
| P p.19: sustentación completa con manifiesto y ampliación cronometrada | Por verificar | No puede acreditarse con el repositorio actual | Preparar entregables anteriores y demostrar ante docente. |

## Recomendaciones, secuencia de trabajo y extras

No se convierten los ejemplos de arquitectura en obligaciones nuevas. La ausencia de estas mejoras opcionales no invalida por sí sola el núcleo.

| Requisito/recomendación del documento | Estado | Evidencia concreta | Qué falta o alcance |
|---|---|---|---|
| P RF-09, p.7: traza de cola de listos y decisión en cada instante (opcional) | Pendiente | API solo devuelve segmentos/métricas; UI sin traza | Extra opcional; no implementado. |
| P RF-10, p.7: CSV de comparación o imagen del diagrama (opcional) | Parcial | UI `exportarInforme` llama `window.print()` | Imprimir/PDF no es CSV ni descarga de imagen; falta formato solicitado si se decide abordar el extra. |
| P RF-11, p.7: dos/tres cargas clásicas sembradas al arrancar (opcional) | Pendiente | UI tiene una carga inicial temporal, no catálogo sembrado; SQL sin INSERT de ejemplo | Los conjuntos existentes en desarrollo no son sembrado reproducible. |
| P p.21: gestión de memoria y ajustes (opcional condicionado) | Pendiente | No hay asignación/fragmentación ni vista de memoria | Fuera del núcleo; no comenzar hasta controles, despliegue y bitácora completos y acuerdo docente. |
| P p.2 y N p.2: multicore, tiempo real, E/S/bloqueos, envejecimiento/multinivel | Cumplido | Modelos y núcleo son CPU única, sin esos subsistemas | Respetado el alcance excluido; no son pendientes obligatorios. El quinto algoritmo es escenario de calidad, no requisito de implementar todos estos extras. |
| P p.16: arquitectura propuesta y ausencia/presencia de gateway | Cumplido | Dos APIs directas, sin gateway; decisiones en bitácora | La arquitectura es propuesta; se justifica la alternativa elegida, no se exige copiarla literalmente. |
| P p.20–21 y N p.10–11: acordar contrato, escribir controles antes, cadena FIFO primero, despliegue temprano | Por verificar | Hay contrato, pruebas y cadena completa en historial Git | No atribuir retrospectivamente una cronología a partir de fuentes actuales. Faltan controles manuales y despliegue temprano; secuencia recomendada no se corrige reescribiendo Git. |
| P p.21: repartir por dominio y ambos integrantes saben compilar/levantar sistema | Por verificar | Git registra aportes de ambos integrantes | La competencia compartida requiere demostración; no basta autoría Git. |
| Extra del equipo: historial persistente privado y quantum recordado por conjunto | Cumplido | PLAN repositorio por sub; PROC columna nullable quantum; UI guardar/cargar; PT/ST y pruebas Angular | La tabla RA exige consultar historial propio aunque el cuadro de arquitectura lo llame opcional. Quantum de conjunto es ampliación opcional aprobada; no sustituye el quantum editable por corrida. |

## Contradicciones y diferencias que requieren acuerdo, no implementación silenciosa

1. P anuncia «diez requisitos, tres opcionales», pero enumera **RF-01 a RF-11**: se auditaron todos (ocho obligatorios y tres opcionales).
2. P p.4 y N p.2 exigen motor Java de referencia/respaldo; P p.17 también rechaza duplicar algoritmos «por si acaso». Se interpreta que falta la referencia solicitada con selección/medición, no que deba eliminarse el requisito explícito. Aclarar con el profesor.
3. P RF-06 coloca promedios en backend, compatible con Java actual; N pide promedios del núcleo y P p.17 propone agregados en C. El sistema cumple la función, pero ese reparto necesita acuerdo.
4. Una sola llamada por corrida en el texto frente a sondeo+ejecución en la ABI actual. No se presenta esta diferencia como cumplida por existir una sola petición HTTP.
5. Encabezados/firmas de ejemplo del PDF difieren de la ABI v1 de ancho fijo, buffers dinámicos, ocio por bandera y estados 0–5; la bitácora registra la evolución, no demuestra aceptación docente.
6. Arquitectura RQ-01 mezcla cinco corridas/mediana y descarte de las tres primeras; no mezclar con el protocolo distinto de Web 2 (30 repeticiones, cuatro tamaños).
7. RA-02 exige retorno a ruta solicitada y redirección automática; la aplicación actual es una pantalla con vistas por signals y login explícito. Funciona, pero no acredita ese criterio literal.
8. La equivalencia RR con quantum grande y FCFS necesita desempates coherentes; no está garantizada para cualquier empate con prioridades diferentes.

## Pruebas ejecutadas en esta auditoría

No se ejecutaron scripts que creen usuarios o escriban en PostgreSQL real. No se reiniciaron servicios ni se modificaron cuentas, contraseñas o datos. Las pruebas HTTP usan H2 y JWKS RSA locales de prueba. Se aprovecha la verificación real previa y la confirmación manual del usuario como antecedentes, sin presentarlas como nuevas ejecuciones.

| Comprobación | Resultado |
|---|---|
| Angular `npm test -- --watch=false` con Node 24.15.0 | 14 pruebas, 3 archivos, todas pasan |
| Angular `npm run build` | Correcto; advertencias previas CSS 5,97 kB/aviso 4 kB y CommonJS transitivo de Keycloak |
| proceso-service, Maven/Java 21 `test` | 7 pruebas, 0 fallos, 0 errores, 0 omitidas; BUILD SUCCESS en informe Maven |
| C en WSL `make -C native test` | Compila con C11, -Wall -Wextra -Werror -pedantic -O2; 34 casos correctos |
| Tiempo incremental de suite C en este equipo | 0,1195 s incluyendo invocación WSL; no es medición en clon/máquina limpia |
| planificador-service en WSL `mvn ... -Dplanificador.biblioteca=... test` | 19 pruebas, 0 fallos, 0 errores, 0 omitidas; biblioteca C real |
| `git diff --check` y sincronización previa | Sin errores de whitespace; HEAD y origin/integracion coinciden antes del commit |

Comandos reproducidos para las pruebas backend/nativas (desde raíz; usar herramientas equivalentes si cambia el equipo):

```powershell
& 'C:/Program Files/JetBrains/IntelliJ IDEA 2026.2.2/plugins/maven-plugin/lib/maven3/bin/mvn.cmd' -f backend/proceso-service/pom.xml test
```

```sh
# Terminal Ubuntu/WSL, desde la raíz Linux del repositorio
export JAVA_HOME="$PWD/backend/planificador-service/target/herramientas/jdk-21.0.13+9"
export PATH="$JAVA_HOME/bin:$PWD/backend/planificador-service/target/herramientas/apache-maven-3.9.11/bin:$PATH"
make -C native test
mvn -f backend/planificador-service/pom.xml \
  -Dmaven.repo.local=/mnt/c/Users/juanf/.m2/repository \
  -Dplanificador.biblioteca="$PWD/native/libplanificador.so" test
```

La biblioteca ya estaba compilada; para un clon nuevo se necesita `make -C native library` antes de JUnit. Las herramientas temporales bajo target no forman parte de Git. Los comandos Angular y sus versiones están en `frontend/README.md`.

## Tres pendientes prioritarios para decidir con el equipo

1. **Cerrar aceptación funcional y testabilidad:** seis controles manuales y matriz completa, desempates visibles/acordados, validación con mensajes por campo y comparación conjunta 4×3 con ganadores de cada métrica. No hace falta otro simulador en TypeScript.
2. **Desplegar el sistema completo reproduciblemente:** Dockerfile multietapa y stack Swarm con límites, importación/provisión segura de Keycloak y prueba desde máquina limpia; medir imagen/arranque y cargar `.so` dentro de ella.
3. **Completar referencia Java y evidencias de calidad:** decidir con el profesor las discrepancias del contrato/C y protocolos, luego medir C frente a Java con ambos bancos; preparar ampliación en vivo y sesiones de usabilidad sin inventar resultados.

No se implementó ninguno de estos pendientes. No se añadieron dependencias a los proyectos para esta auditoría; el lector PDF se descargó únicamente bajo `.angular/`, ignorado por Git. Los documentos originales y el diseño de Angular permanecen intactos.
