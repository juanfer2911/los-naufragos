# Evidencias de seguridad y usabilidad

**Corte:** 2026-11-16. Este registro distingue controles visibles en el código, verificaciones realizadas y sesiones humanas pendientes. No se incluyen tokens, credenciales ni datos personales.

## Evidencias registradas

| Verificación | Resultado | Evidencia |
|---|---|---|
| Stack Swarm en estado estable | 5 servicios `1/1` | `docker stack services naufragos` tras el despliegue desde volumen nuevo. |
| Catálogo OIDC del realm | HTTP 200 | `GET http://localhost:8080/realms/planificador-realm/.well-known/openid-configuration`. |
| Frontend publicado | HTTP 200 | `GET http://localhost:4200`. |
| API sin token | HTTP 401 | GET `/algoritmos` y GET `/conjuntos`; se rechaza acceso anónimo. |
| Bases del stack | `keycloak`, `planificador_db`, `proceso_db` presentes | Consulta del catálogo PostgreSQL del servicio Swarm. |
| Vida del access token | 60 segundos | Realm configurado y consultado mediante Keycloak Admin CLI; `accessTokenLifespan: 60`. |

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

Los resultados HTTP 401 prueban el rechazo sin token; **no** prueban que un token válido, una audiencia incorrecta, expiración, rol o aislamiento entre usuarios se comporte correctamente en toda la aplicación. Registrar cuando se haga:

- estudiante: consultar conjuntos, intentar crear/eliminar y verificar 403;
- docente: crear y eliminar conjunto;
- token expirado y renovación/logout con vida de 60 segundos;
- abrir una ruta protegida tras logout y usar Atrás;
- dos usuarios consultan historial y no ven planificaciones del otro.

La prueba de expiración del requisito académico también solicita evidenciar el retorno de navegación tras logout. La duración de 60 segundos ya está configurada, pero ese recorrido aún no se registra como completado.

## Registro de las tres sesiones RQ-03

Requisito de [auditoria-requisitos.md](auditoria-requisitos.md): tres personas ajenas al equipo, comparar una carga de cinco procesos, máximo 12 interacciones, máximo 5 minutos y cero preguntas. Usar códigos anónimos U1–U3; no anotar nombres.

| Sesión | Fecha | Tarea completada | Interacciones | Tiempo | Preguntas del participante | Ayuda del observador | Incidentes/notas |
|---|---|---|---:|---:|---:|---|---|
| U1 | Pendiente | Pendiente | — | — | — | — | — |
| U2 | Pendiente | Pendiente | — | — | — | — | — |
| U3 | Pendiente | Pendiente | — | — | — | — | — |

No marcar una sesión exitosa hasta registrar que cumplió todos los umbrales. Las pruebas automatizadas no reemplazan estas sesiones.
