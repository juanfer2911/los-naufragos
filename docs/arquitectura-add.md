# Borrador de arquitectura ADD

**Estado:** borrador para revisión. Describe los límites acordados por los documentos del proyecto y las decisiones que todavía requieren confirmación. No define contratos REST ni interfaces C/JNA definitivos.

## 1. Drivers arquitectónicos

### Propósito

Simular la planificación de CPU sobre conjuntos de procesos, presentar diagramas de Gantt y métricas, y permitir comparar algoritmos sobre la misma carga. No se ejecutan procesos reales del sistema operativo.

### Capacidades funcionales principales

- Crear, guardar, consultar y reutilizar conjuntos de procesos.
- Simular FCFS/FIFO, SJF sin desalojo, SRTF con desalojo y Round Robin con quantum configurable.
- Devolver la línea de tiempo completa, incluidos los intervalos de CPU ociosa, métricas por proceso y promedios de espera, retorno y respuesta.
- Consultar dinámicamente los algoritmos disponibles y comparar la carga con los cuatro algoritmos principales.
- Autenticar con Keycloak, aplicar autorización por rol en el backend y conservar el historial de corridas asociado al usuario.

### Restricciones confirmadas

- Angular en `frontend/`, con componentes standalone y signals.
- Dos microservicios Spring Boot: `proceso-service` y `planificador-service`. No se incorpora un tercer servicio.
- Núcleo de planificación en C, como biblioteca compartida Linux `libplanificador.so`, integrado con JNA desde `planificador-service`.
- Implementación equivalente en Java para referencia/benchmark y ruta seleccionable por configuración.
- Keycloak como proveedor de identidad OAuth 2.0/OIDC; no se construye un sistema propio de cuentas.
- Despliegue contenedorizado con Kubernetes o Docker Swarm; la elección del orquestador sigue pendiente.
- `proceso-service` es dueño de la persistencia de procesos y conjuntos.
- El frontend presenta los resultados, pero no ejecuta algoritmos ni calcula promedios.

## 2. Atributos de calidad relevantes

- **Modificabilidad (Web 2, RQ-01):** agregar un algoritmo debe requerir cambios en el código C y su cabecera, sin cambios en Angular, Java de dominio ni OpenAPI; el algoritmo se publica mediante el catálogo que informa la biblioteca.
- **Testabilidad (Web 2, RQ-02):** seis conjuntos de control resueltos manualmente deben verificar segmentos exactos y promedios; reglas de desempate documentadas y visibles.
- **Usabilidad (Web 2, RQ-03):** tres personas ajenas al equipo deben completar la comparación dentro de los límites documentados de interacciones y tiempo, sin ayuda.
- **Desempeño, testabilidad y facilidad de despliegue (Arquitectura de Computadores):** medir C frente a Java, probar la biblioteca sin levantar el sistema y desplegar desde una máquina limpia con un único comando. Los protocolos de medición están pendientes de reconciliación.
- **Seguridad:** cada microservicio protege sus endpoints mediante validación JWT y autorización de servidor; los detalles operativos se especifican en los documentos fuente y deben reflejarse en el diseño de implementación.

## 3. Descomposición de componentes

| Componente | Responsabilidad | Datos que posee o procesa | Dependencias principales |
|---|---|---|---|
| `frontend/` (Angular) | Captura y valida la experiencia de usuario, inicia sesión, solicita operaciones y representa conjuntos, Gantt, métricas e historial. | Estado de interfaz; no es fuente de verdad de procesos ni de resultados. | Keycloak para OIDC; API de `proceso-service` y `planificador-service`. |
| `backend/proceso-service/` (Spring Boot) | Dueño del dominio de procesos y conjuntos: registrar, validar, consultar, guardar y eliminar según rol. | Procesos y conjuntos persistidos en su propia base de datos. | Base de datos propia; Keycloak/JWKS para validación de tokens. |
| `backend/planificador-service/` (Spring Boot) | Publica algoritmos disponibles, recibe solicitudes de planificación/comparación, selecciona el motor configurado y adapta resultados al dominio/API. | Solicitudes y resultados; el historial es obligatorio, pero su ubicación de persistencia no está decidida. | Biblioteca C por JNA o implementación Java de referencia; Keycloak/JWKS; potencialmente `proceso-service`, según la decisión de intercambio de conjuntos. |
| `native/` | Aloja el núcleo de planificación en C y la biblioteca compartida `libplanificador.so`; produce segmentos y métricas en una llamada por corrida. | Estructuras de entrada y salida de una ejecución, conforme al contrato que se acuerde. | Compilador C en la etapa de construcción; adaptador JNA del planificador en ejecución. |
| Adaptador nativo dentro de `planificador-service` | Encapsula JNA, mapea las estructuras nativas y traduce códigos de retorno a errores de dominio/API. | No expone tipos C a controladores ni al dominio restante. | `libplanificador.so` y contrato C/JNA, ambos aún por fijar. |
| Motor Java de referencia dentro de `planificador-service` | Implementa resultados equivalentes para benchmark y respaldo seleccionable. | Mismos procesos, reglas y forma conceptual del resultado que el motor C. | Configuración de selección y pruebas compartidas de resultados. |
| Keycloak | Proveedor externo de identidad: realm, clientes, roles y usuarios de prueba versionados/importables. | Identidades y configuración del realm. | Aplicación Angular y ambos microservicios como clientes/resource servers según corresponda. |
| Persistencia de `proceso-service` | Conserva conjuntos y procesos tras reiniciar contenedores. | Datos de entrada del dominio. | `proceso-service`. |
| Persistencia de historial (por decidir) | Conserva corridas y permite consultar el historial propio. | `sub` del usuario, algoritmo, conjunto y datos de corrida que se definan. | Ubicación no decidida: podría estar en `planificador-service` u otra alternativa que se justifique sin crear un tercer microservicio. |
| `infra/` | Despliegue reproducible del sistema completo, configuración de contenedores y límites de recursos. | Configuración de despliegue; no es autoridad de datos de negocio. | Angular, ambos microservicios, Keycloak, bases de datos y biblioteca nativa. |
| `tests/` | Agrupa las pruebas transversales del sistema; las pruebas unitarias nativas pueden vivir junto al componente C si se acuerda esa organización. | Casos de control y evidencias de prueba. | Componentes probados; suites nativas deben poder ejecutarse sin Spring, Angular ni orquestador. |

Los nombres de carpetas `frontend/` y `tests/` ya existen en el repositorio y estaban vacíos al preparar este borrador. Las carpetas de backend, native e infra también estaban vacías. Esta descripción no implica que existan ya proyectos ejecutables.

## 4. Dependencias y comunicaciones

### Flujos principales

1. El navegador usa Angular para iniciar OIDC con Keycloak mediante cliente público y PKCE. El token se adjunta a las llamadas API.
2. Angular se comunica por HTTP con `proceso-service` para operaciones de procesos y conjuntos, y con `planificador-service` para algoritmos, planificación, comparación e historial. La forma exacta de las rutas y los cuerpos todavía no está definida.
3. Cada microservicio valida por sí mismo los tokens que recibe; el frontend no es autoridad de autorización.
4. `planificador-service` necesita los datos del conjunto para una corrida. Dos alternativas siguen abiertas: el propio servicio consulta `proceso-service`, o Angular envía al planificador la carga completa. No se fija aquí ninguna.
5. Una vez que el planificador tiene los procesos, selecciona por configuración el motor Java o el adaptador JNA que invoca la biblioteca C. El resultado se transforma a un DTO genérico y se devuelve al cliente; Angular lo presenta sin recalcular métricas.
6. El despliegue agrupa la aplicación Angular, los dos servicios, Keycloak, las bases de datos que se definan y la biblioteca nativa dentro de la imagen de `planificador-service`, usando un único orquestador aún por elegir.

### Límites de dependencia

- Angular no accede directamente a bases de datos ni conoce las estructuras C/JNA.
- `proceso-service` valida y persiste procesos, pero no conoce ni ejecuta algoritmos.
- Solo el adaptador nativo de `planificador-service` conoce la frontera de memoria/JNA.
- El identificador de algoritmo debe poder viajar como texto y validarse contra el catálogo dinámico; el formato y el comportamiento exacto del catálogo deben concretarse en el contrato.
- La frontera entre servicios se documentará en OpenAPI cuando se resuelvan las decisiones pendientes. Este borrador no congela rutas, esquemas ni códigos HTTP nuevos.

## 5. Decisiones de arquitectura pendientes

1. Resolver con el docente la diferencia entre el mínimo general de tres microservicios y el diseño específico de dos. Este borrador conserva los dos servicios del equipo y no propone un tercero.
2. Confirmar si prioridad es solo un atributo/regla de desempate o también un algoritmo seleccionable; definir el sentido de prioridad y todos los desempates.
3. Decidir si `planificador-service` recupera conjuntos desde `proceso-service` o recibe la carga completa en la solicitud.
4. Decidir dónde persiste el historial y qué datos exactos se retienen para consulta propia.
5. Acordar el alcance de “comparar cuatro algoritmos” si el catálogo puede ampliarse dinámicamente.
6. Seleccionar Kubernetes o Docker Swarm.
7. Congelar los contratos REST/OpenAPI y C/JNA: estructuras y tamaños máximos, asignación/liberación de memoria, códigos de error y límites.
8. Reconciliar los protocolos de benchmark descritos por Web 2 y Arquitectura de Computadores, incluida la cantidad de corridas de calentamiento y medición.
9. Determinar versiones compatibles del stack y motor de base de datos si no están fijadas por el curso o el equipo.
10. Evaluar si se necesita un API Gateway; no se presupone su incorporación.

## 6. Referencias

- `docs/Enunciado general_PI.pdf`
- `docs/- Los Naufragos - Planificador de procesos.pdf`
- `docs/Los Naufragos - Nucleo de planificacion.pdf`
- `docs/Guia_maestra_Copilot_Planificador_Los_Naufragos.docx`
- `docs/wireframes/`
