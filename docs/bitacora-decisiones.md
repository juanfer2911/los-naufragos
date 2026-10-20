# Borrador de bitácora de decisiones

**Estado:** borrador. Distingue decisiones confirmadas por los documentos y por el alcance aprobado para esta etapa de cuestiones todavía abiertas. No constituye una aprobación de contratos técnicos definitivos.

## Decisiones confirmadas

| ID | Decisión | Alternativa considerada o límite | Justificación / requisito relacionado | Estado |
|---|---|---|---|---|
| D-01 | Mantener dos microservicios: `proceso-service` y `planificador-service`; no crear un tercero. | El enunciado general indica un mínimo de tres microservicios. | La ficha específica del equipo define dos servicios y el usuario confirma expresamente ese alcance. La discrepancia con el requisito general queda pendiente de aclaración docente. | Confirmada para la estructura del equipo; no resuelve el requisito general. |
| D-02 | Usar Angular bajo `frontend/` y reservar `tests/` para pruebas. | Las denominaciones `fronted/` y `test/` no son las acordadas. | Consistencia con la estructura definida y el README. Al inspeccionar el repositorio, `frontend/` y `tests/` ya existían y estaban vacías; no fue necesario renombrar ni trasladar contenido. | Confirmada; nombres ya presentes. |
| D-03 | Preservar intactos los documentos originales de `docs/`. | No reemplazar ni reescribir los PDF, DOCX ni wireframes. | Son las fuentes del proyecto; esta etapa solo agrega borradores técnicos. | Confirmada y aplicada. |
| D-04 | Conservar la separación de responsabilidades: Angular presenta; `proceso-service` posee procesos y conjuntos; `planificador-service` ejecuta la planificación e integra los motores. | No trasladar lógica de planificación a Angular ni ejecución de algoritmos a `proceso-service`. | Requisitos del proyecto y escenario de modificabilidad. | Confirmada a nivel de arquitectura, no de implementación. |
| D-05 | Mantener el núcleo nativo C (`libplanificador.so`) integrado por JNA, junto a una implementación Java equivalente seleccionable por configuración. | No sustituir el componente C por Java ni duplicar la selección en el frontend. | Requisitos del núcleo nativo, benchmark y respaldo/configuración de Web 2. | Confirmada como dirección arquitectónica; contrato pendiente. |
| D-06 | Usar Keycloak para OAuth 2.0/OIDC, con autorización en los servicios, no un sistema propio de cuentas. | No implementar credenciales propias ni confiar en ocultar controles del frontend como autorización. | Requisitos comunes de Web 2 y matriz de roles del proyecto. | Confirmada como dirección arquitectónica. |
| D-07 | Mantener identificadores de algoritmos dinámicos y publicar el catálogo originado en la biblioteca, evitando listas cerradas en Angular/Java. | Enum/lista fija que obligue a cambiar múltiples módulos al añadir algoritmos. | RQ-01 de modificabilidad. | Confirmada como principio; contrato y definición exacta de comparación pendientes. |
| D-08 | No crear todavía `openapi.yaml` ni fijar interfaces C/JNA definitivas. | Congelar contratos antes de resolver decisiones que afectan sus campos, memoria, límites y errores. | Instrucción expresa del usuario y requisito de acordar contratos entre frentes. | Confirmada para esta etapa. |
| D-09 | No implementar en esta etapa aplicaciones, algoritmos, servicios ni dependencias. | Adelantar código antes de la revisión de estructura y documentación. | Secuencia por etapas aprobada por el usuario. | Confirmada y aplicada. |

## Cuestiones pendientes de confirmación

| ID | Cuestión | Evidencia / alternativas abiertas | Próxima confirmación necesaria |
|---|---|---|---|
| P-01 | Número de microservicios exigido para la evaluación. | El enunciado general de Web 2 exige un mínimo de tres; la ficha específica de Los Náufragos define `proceso-service` y `planificador-service`. El equipo mantiene el diseño de dos por instrucción expresa; no se propone ni crea un tercero. | Consultar al docente cómo aplicar el requisito general al proyecto específico de dos servicios. |
| P-02 | Alcance del atributo prioridad. | Se exige el campo y se mencionan desempates por prioridad, pero el alcance principal enumera cuatro algoritmos. Algunos ejemplos de catálogo C incluyen también `prioridad`. | Confirmar si existe un algoritmo de prioridad seleccionable y definir qué valor gana, además de desempates deterministas para llegada, ráfaga y prioridad iguales. |
| P-03 | Protocolos de benchmark entre entregables. | Web 2 pide tamaños de 50, 500, 5.000 y 50.000 procesos, 30 repeticiones y mediana. Arquitectura pide SRTF con 10.000 procesos y cinco corridas por lado, pero el mismo protocolo menciona descartar las tres primeras por calentamiento JIT; cinco corridas totales dejarían solo dos mediciones posteriores. El documento general de Arquitectura también describe la matriz de tamaños de Web 2. | Confirmar si se ejecutan ambos protocolos separados y aclarar cuántas corridas son de calentamiento y cuántas se incluyen en la mediana. |
| P-04 | Quién suministra los procesos a `planificador-service`. | El servicio puede consultar el conjunto a `proceso-service`, o Angular puede enviar la carga completa. | Comparar acoplamiento entre servicios y tamaño/duplicación de solicitudes; registrar la decisión antes de definir el contrato. |
| P-05 | Persistencia del historial de corridas. | El historial propio es funcionalmente obligatorio; su ubicación no está fijada. Puede justificarse dentro de `planificador-service` u otra solución sin añadir un microservicio. | Decidir dueño, retención, campos y consulta de historial propio, incluyendo uso del claim `sub`. |
| P-06 | Comparación y catálogo ampliable. | Se pide comparar los cuatro algoritmos principales, mientras que el catálogo debe reflejar algoritmos añadidos. No se ha fijado si “comparar” significa siempre esos cuatro o todos los algoritmos disponibles. | Definir el alcance de comparación sin impedir que un quinto algoritmo aparezca dinámicamente. |
| P-07 | Contrato de la biblioteca C/JNA. | Falta fijar firmas exactas, mapeo/alineación, tamaños de arreglos, límites máximos, propiedad y liberación de memoria, códigos de error y tratamiento de fallos. | Acordar y documentar el contrato compartido antes de implementar la biblioteca o el adaptador. |
| P-08 | Contratos REST/OpenAPI. | Se requieren al menos `GET /algoritmos` y `POST /planificaciones`, pero faltan rutas y JSON de procesos/conjuntos, comparación, errores e historial. | Resolver P-04 a P-06 y luego acordar y revisar el OpenAPI; no se crea contrato en esta etapa. |
| P-09 | Orquestador de despliegue. | Los documentos permiten Kubernetes o Docker Swarm. | Seleccionar uno y justificarlo en función del despliegue completo y del esfuerzo del equipo. |
| P-10 | Versiones y persistencia tecnológica. | No están fijadas en la estructura inicial las versiones de Angular, Java, Spring Boot, JNA, Keycloak, ni el motor de base de datos. | Revisar requisitos del curso/equipo antes de seleccionar versiones compatibles y registrarlas. |
| P-11 | Correspondencia entre wireframes y requisitos. | Los wireframes muestran un selector fijo de cuatro algoritmos, pantallas de resultados por algoritmo y un enlace “Crear cuenta”; los requisitos piden catálogo dinámico, comparación y autenticación delegada en Keycloak. | Confirmar que los wireframes son ilustrativos y ajustar el diseño visual tras definir el flujo de comparación y autenticación. |
| P-12 | Inicialización de Git. | `git rev-parse --is-inside-work-tree` falla con “not a git repository”; no hay `.git` en la raíz. | El responsable del repositorio debe confirmar que esta carpeta es la raíz prevista y ejecutar `git init` antes de crear commits. Si el proyecto ya está bajo otro repositorio, trabajar desde esa raíz en vez de inicializar uno anidado. |
| P-13 | Incorporación de un API Gateway. | Los documentos sugieren evaluarlo, pero señalan que con dos servicios podría añadir complejidad sin beneficio suficiente. | Evaluar necesidad y costo; registrar la decisión sin tratar el Gateway como un tercer microservicio de dominio. |

## Estado del repositorio al preparar este borrador

- La raíz de trabajo no está inicializada como repositorio Git.
- `frontend/` y `tests/` ya existían y no tenían archivos; no hubo renombres ni traslado de contenido.
- `backend/`, `native/` e `infra/` estaban vacíos.
- Se agregan únicamente este borrador y `docs/arquitectura-add.md`; los documentos originales no se modifican.

Para inicializar Git, una vez confirmado que esta carpeta es la raíz correcta, ejecutar desde `C:\Users\juanf\Documents\los-naufragos`:

```powershell
git init
git status
```

Después de revisar `.gitignore` y confirmar qué archivos deben versionarse, se podrán preparar y crear commits. No se inicializó Git ni se creó ningún commit en esta etapa.
