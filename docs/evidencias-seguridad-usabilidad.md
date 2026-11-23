# Evidencias de seguridad y usabilidad

**Corte:** 2026-11-18. Este registro distingue controles visibles en el código, verificaciones realizadas y sesiones humanas pendientes. No se incluyen tokens, credenciales ni datos personales.

## Evidencias registradas

| Verificación | Resultado | Evidencia |
|---|---|---|
| Stack Swarm en estado estable | 5 servicios `1/1` | `docker stack services naufragos` tras el despliegue desde volumen nuevo. |
| Catálogo OIDC del realm | HTTP 200 | `GET http://localhost:8080/realms/planificador-realm/.well-known/openid-configuration`. |
| Frontend publicado | HTTP 200 | `GET http://localhost:4200`. |
| API sin token | HTTP 401 | GET `/algoritmos` y GET `/conjuntos`; se rechaza acceso anónimo. |
| Bases del stack | `keycloak`, `planificador_db`, `proceso_db` presentes | Consulta del catálogo PostgreSQL del servicio Swarm. |
| Vida del access token | 60 segundos | Realm configurado y consultado mediante Keycloak Admin CLI; `accessTokenLifespan: 60`. |
| Compose local | Configuración válida | `docker compose --env-file infra/.env -f infra/docker-compose.yml config --quiet`, 2026-11-18. |
| Build de proceso-service | Correcto; 7 pruebas, 0 fallos/errores/omisiones | `docker build -t naufragos-proceso:local -f backend/proceso-service/Dockerfile .`, 2026-11-18. |
| Build del planificador | Correcto; 31 pruebas, 0 fallos/errores/omisiones | `docker build -t naufragos-planificador:local -f backend/planificador-service/Dockerfile .`; incluye 21 pruebas C/JNA, 9 HTTP y 1 de contexto, 2026-11-18. |
| Build Angular | Correcto; suite unitaria y compilación terminaron con código 0 | `docker build -t naufragos-frontend:local -f frontend/Dockerfile frontend`, 2026-11-18. La suite contiene 17 casos actualmente. |
| Ruta protegida | Pruebas unitarias cubren URL directa sin sesión y sincronización URL/vista | `frontend/src/app/app.spec.ts`; aún falta confirmar el retorno directo con Keycloak real en una sesión limpia. |

## Recorrido manual reportado por el usuario (2026-11-18)

La persona que opera la aplicación informó los siguientes resultados durante una sesión interactiva. Son observaciones de una persona; no se presentan como pruebas automatizadas, aceptación del docente ni como las tres sesiones RQ-03. No se capturaron artefactos ni se midieron formalmente interacciones o tiempos.

| Verificación | Resultado reportado |
|---|---|
| Gantt con ocho procesos, zoom al 100 % | Se confirmó que se distinguen los procesos/segmentos y que no requiere desplazamiento horizontal. |
| Métricas y comparación | Se ven las métricas por proceso, promedios y comparación de los algoritmos con sus mejores resultados. |
| Entrada inválida en la interfaz | La interfaz impide ráfaga 0, llegada -1 y quantum 0. Esto verifica el bloqueo del formulario; no sustituye las respuestas HTTP 400 de la API. |
| Ciclo docente/estudiante de bancos | Docente creó un banco de ocho procesos; permaneció tras recargar; estudiante pudo consultarlo/cargarlo sin controles de escritura; docente lo eliminó y confirmó que los bancos anteriores seguían presentes. |
| Logout y navegación Atrás | Después del logout, Atrás devolvió al inicio de sesión. |
| Sesión después de un minuto | Continuó autenticada tras dejar la aplicación abierta e inactiva alrededor de un minuto. No se inspeccionó el refresh del token; el resultado es compatible con renovación, pero no la demuestra por sí solo. |
| Historial | Una planificación siguió visible tras recargar; al entrar con el otro usuario, no apareció en su historial. |

## Controles observables en código/configuración

| Control | Implementación |
|---|---|
| Cliente Angular | OIDC estándar, PKCE S256, cliente público; `frontend/src/app/auth.service.ts` y `backend/planificador-service/dev/keycloak-realm.json`. |
| Renovación/expiración | Angular llama `updateToken(30)` y limpia estado de sesión ante error de renovación; `frontend/src/app/auth.service.ts`. |
| Validación API | Firma con JWKS, emisor, expiración estándar, `sub` no vacío y audiencia específica; `backend/*-service/src/main/java/**/Seguridad.java`. |
| Autorización | Todos los endpoints requieren autenticación. Crear/eliminar conjuntos requiere `ROLE_docente`; historial del planificador filtra por `sub`. |
| CORS | Orígenes explícitos; la configuración rechaza comodines. |
| Secretos de Swarm | PostgreSQL, Keycloak y APIs reciben credenciales desde secretos montados bajo `/run/secrets`; no se imprimen aquí. |

## Comprobaciones funcionales todavía necesarias

Los resultados HTTP 401 prueban el rechazo sin token. El recorrido reportado arriba aporta evidencia manual de roles, renovación, retorno tras logout e historial privado, pero no incluye capturas, respuestas HTTP ni tokens de prueba. Aún falta un registro reproducible y sin secretos para cada servicio, incluyendo:

- estado HTTP 403 del backend al intentar escritura como estudiante (los controles ocultos por la UI no prueban por sí solos el 403);
- token expirado y renovación con vida de 60 segundos, comprobando el evento de expiración;
- rechazo con JWT firmado y luego alterado en ambos servicios.

La configuración de 60 segundos y el regreso al inicio de sesión mediante Atrás se comprobaron en el recorrido reportado; la expiración/renovación sigue pendiente de un registro reproducible.

## Registro de las tres sesiones RQ-03

Requisito de [auditoria-requisitos.md](auditoria-requisitos.md): tres personas ajenas al equipo, comparar una carga de cinco procesos, máximo 12 interacciones, máximo 5 minutos y cero preguntas. Usar códigos anónimos U1–U3; no anotar nombres.

| Sesión | Fecha | Tarea completada | Interacciones | Tiempo | Preguntas del participante | Ayuda del observador | Incidentes/notas |
|---|---|---|---:|---:|---:|---|---|
| U1 | Pendiente | Pendiente | — | — | — | — | — |
| U2 | Pendiente | Pendiente | — | — | — | — | — |
| U3 | Pendiente | Pendiente | — | — | — | — | — |

No marcar una sesión exitosa hasta registrar que cumplió todos los umbrales. Las pruebas automatizadas no reemplazan estas sesiones.

## Verificaciones adicionales — 2026-11-19

Se repitieron las comprobaciones con Chrome automatizado sobre el stack Swarm local y una cuenta temporal de estudiante, eliminada al terminar. No se guardaron tokens ni credenciales.

| Control | Resultado observado |
|---|---|
| Token válido en ambas APIs | HTTP 200 en `GET /algoritmos` y `GET /conjuntos` |
| Estudiante intenta escribir bancos | HTTP 403 en `POST /conjuntos` y `DELETE /conjuntos/{id}` |
| Firma del JWT alterada | HTTP 401 en ambas APIs |
| Renovación proactiva | Con vida configurada de 60 s, un GET desde Angular cuando quedaban menos de 30 s envió un token distinto con `exp` posterior; la sesión siguió activa |

La prueba verificó la renovación proactiva antes del vencimiento; no esperó a que el token expirara ni comprobó el flujo de renovación fallida. Esa parte permanece pendiente. Los GET iniciales confirmaron acceso con token válido; las operaciones de escritura denegadas no modificaron conjuntos. La cuenta temporal fue eliminada y no se dejó token en archivos.

Las imágenes runtime y el redeploy integral se registran en [el informe de benchmarks](../tests/benchmarks/README.md). La medición de 97,718 s usó el volumen de datos existente; no es una mediana ni una puesta en marcha con volumen vacío.
La captura automatizada del Gantt con ocho procesos, viewport de 1366 px y sin desplazamiento horizontal está en `evidencia-gantt-8-procesos-1366.png`. Se verificaron ocho etiquetas, anchos de segmento suficientes y ausencia de recorte antes de guardar la imagen.
## Guion para las sesiones RQ-03 pendientes

Usar la misma carga preparada de cinco procesos para U1-U3 y una cuenta de estudiante sin privilegios de escritura. Leer una sola vez esta tarea: **"Compara los algoritmos con este conjunto y dime cuál tiene el menor promedio de espera."** Iniciar el cronómetro al terminar de leerla. No explicar la interfaz, no hacer preguntas ni dar pistas; registrar cada acción visible, el tiempo, las preguntas espontáneas, la ayuda y el resultado. Detener el tiempo cuando la persona identifica el ganador o al cumplir cinco minutos. No registrar nombres ni grabar datos personales.

El guion solo prepara la ejecución; no sustituye ni anticipa las sesiones. Respetar el límite de 12 interacciones y cero preguntas del participante indicado por RQ-03.