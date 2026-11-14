# Backend REST: ejecución y conexión de Angular

## Contrato y alcance

Se conserva `docs/openapi.yaml` sin cambios. Todas las rutas requieren `Authorization: Bearer <access_token>` de Keycloak. El catálogo procede de C.

| Método | Ruta | Éxito |
|---|---|---|
| GET | `/algoritmos` | 200, `{ "algorithms": [{ "id": "fcfs" }, ...] }` |
| POST | `/planificaciones` | 201, `PlanificationRecord` guardado |
| GET | `/historial` | 200, `{ "items": [...] }` del usuario autenticado |
| GET | `/historial/{planificacionId}` | 200, registro propio; 404 si no existe o pertenece a otro usuario |

`/comparaciones` sigue pendiente. Las rutas de conjuntos corresponden a `proceso-service`. No hay sistema de usuarios local.

## Infraestructura existente

Docker Desktop debe estar iniciado, con motor Linux e integración de Ubuntu/WSL habilitada. Desde la raíz:

```sh
# Solo si todavía no existe; no sobrescribir la configuración del equipo.
test -f infra/.env || cp infra/.env.example infra/.env
# Editar infra/.env y establecer contraseñas locales antes de continuar.
docker compose --env-file infra/.env -f infra/docker-compose.yml up -d postgres keycloak
docker compose --env-file infra/.env -f infra/docker-compose.yml ps
```

Compose crea `planificador_db`, propiedad de `planificador_user`. Su contraseña es `PLANIFICADOR_DB_PASSWORD` de `infra/.env`. El script inicial solo se ejecuta al crear el volumen por primera vez: con un volumen existente, revisar usuarios/bases sin borrar datos. No ejecutar `down -v` para resolver contraseñas.

PostgreSQL usa localhost:5432 y Keycloak localhost:8080. Ambos deben ser alcanzables desde WSL. En este equipo WSL funciona en modo NAT: usar el [override limitado a la interfaz WSL](verificacion-real.md), que conserva localhost para Windows. El emisor debe coincidir exactamente con el `iss` de los tokens; puede ser distinto de la dirección de JWKS accesible desde Java.

## Keycloak

En `http://localhost:8080`, iniciar la consola administrativa con las credenciales de `infra/.env`.

Si aún no existe `planificador-realm`, crearlo importando [dev/keycloak-realm.json](../dev/keycloak-realm.json). Es una plantilla local sin usuarios ni contraseñas: cliente público `angular-desarrollo`, Authorization Code + PKCE S256, redirect localhost:4200 y audiencia `planificador-service` en los access tokens. No habilita password grant.

Si la compañera ya configuró un realm/cliente, **no importarlo ni reemplazarlo**. Reutilizarlo y configurar:

1. `KEYCLOAK_ISSUER_URI` con el emisor real.
2. `KEYCLOAK_JWK_SET_URI` con su endpoint de certificados.
3. Un mapper de audiencia que incluya `planificador-service` en el access token de Angular, o `KEYCLOAK_AUDIENCE` con la audiencia ya acordada.
4. Redirect URI y origen explícitos de Angular.

Los usuarios y contraseñas de desarrollo se crean en Keycloak. Angular obtiene un access token mediante OIDC y lo envía en `Authorization`; no usar el ID token. Para curl puede usarse el access token de una sesión Angular. El backend no implementa login ni recibe contraseñas de usuarios.

Se valida firma RSA/JWKS, emisor, audiencia, fechas y presencia de `sub`/`exp`. El dueño de la corrida sale exclusivamente del `sub` validado, nunca de `userId`, consultas o campos del cuerpo.

## Variables e inicio en WSL

Copiar `.env.example` de este microservicio a `.env` solo si no existe, editarlo y exportar sus variables en la terminal de Java. Spring no carga `.env` automáticamente. No versionar contraseñas.

| Variable | Valor local / significado |
|---|---|
| `SERVER_PORT` | `8081`, evita colisión con Keycloak |
| `PLANIFICADOR_DB_URL` | `jdbc:postgresql://localhost:5432/planificador_db` |
| `PLANIFICADOR_DB_USER` | `planificador_user` |
| `PLANIFICADOR_DB_PASSWORD` | Obligatoria; la misma que utiliza Compose |
| `KEYCLOAK_ISSUER_URI` | `http://localhost:8080/realms/planificador-realm` |
| `KEYCLOAK_JWK_SET_URI` | URL del realm terminada en `/protocol/openid-connect/certs` |
| `KEYCLOAK_AUDIENCE` | `planificador-service` |
| `PLANIFICADOR_BIBLIOTECA` | Ruta Linux absoluta de `native/libplanificador.so` |
| `FRONTEND_ORIGINS` | `http://localhost:4200`; varios orígenes explícitos separados por coma |

Desde la raíz, con JDK 21 y Maven Linux configurados según el README:

```sh
set -a
. backend/planificador-service/.env
set +a
mvn -f backend/planificador-service/pom.xml spring-boot:run
```

Usar el `.so` ya compilado en `native/`, sin copiarlo. Desde IntelliJ ejecutar la clase Spring con JDK 21 WSL y las mismas variables. Base URL de Angular: `http://localhost:8081`.

Al arrancar se crea, si falta, una tabla `planificaciones` y su índice mediante `schema.sql`: ID, propietario, fecha y snapshot JSON de entrada/resultados. El snapshot conserva además `quantum` para Round Robin; se recupera internamente al leer el historial y se omite de la respuesta HTTP para mantener el contrato existente. Los registros anteriores sin ese dato se leen con quantum nulo, sin migrarlos ni inventar un valor. Cada insert se confirma antes del 201. No se duplican tablas de usuarios ni procesos de otro servicio. H2 es exclusivamente una dependencia de pruebas; producción requiere PostgreSQL.

## Solicitudes y respuestas

[ejemplos-http.json](ejemplos-http.json) contiene respuestas capturadas por HTTP, incluido un 201 con ID/fecha reales del ensayo, catálogo, historial y un error 401. El archivo identifica el entorno de pruebas (H2/JWKS local/C), sin presentarlo como captura de PostgreSQL/Keycloak ni incluir tokens.

```sh
# TOKEN contiene un access token vigente de Keycloak.
curl -i http://localhost:8081/algoritmos -H "Authorization: Bearer $TOKEN"

curl -i http://localhost:8081/planificaciones \
  -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
  --data '{"algorithmId":"fcfs","processes":[{"pid":1,"arrivalTime":1,"burstTime":5,"priority":0},{"pid":2,"arrivalTime":2,"burstTime":1,"priority":0}]}'

curl -i http://localhost:8081/historial -H "Authorization: Bearer $TOKEN"
curl -i "http://localhost:8081/historial/$PLANIFICATION_ID" -H "Authorization: Bearer $TOKEN"
```

Para RR añadir `"quantum":2`; es obligatorio cuando el catálogo C indica que el algoritmo lo necesita. `sourceSetId` es opcional y se conserva sin consultar otro servicio. Se valida llegada ≥ 0, ráfaga > 0, PID ≥ 0 y único, quantum ≥ 1 si se envía, campos obligatorios y tipos enteros. Tiempos: enteros de 64 bits; PID/prioridad: 32 bits. En Angular evitar números mayores al rango entero exacto de Number hasta acordar límites compartidos.

Los segmentos de ejecución tienen `kind`, `pid`, `start`, `end`; los ociosos solo `kind:"idle"`, `start`, `end`. Backend calcula `completionTime` y promedios. La respuesta incluye `processes`, `planificationId` y `createdAt`, sin propietario. `sourceSetId` se omite si no se envió.

Errores: `{ "code": "...", "message": "..." }`. JWT inválido/ausente: 401 `UNAUTHORIZED`; entrada inválida: 400 `BAD_REQUEST`; historial inexistente o ajeno: 404 `NOT_FOUND`; fallo interno: 500 `SERVER_ERROR`. No se devuelven trazas ni detalles de conexión.

CORS permite solo los orígenes configurados, GET/POST/OPTIONS y Authorization/Content-Type. Sin comodines ni cookies de sesión. Orígenes no permitidos reciben rechazo del preflight. CORS no sustituye JWT.

## Verificación y límites

La suite HTTP levanta Tomcat real, genera JWT RSA, sirve claves públicas desde un JWKS local y llama a C real. H2 en modo PostgreSQL comprueba inserts/consultas y aislamiento por `sub`. Esto **no verifica PostgreSQL ni Keycloak reales**.

```sh
mvn -f backend/planificador-service/pom.xml \
  -Dplanificador.biblioteca="$PWD/native/libplanificador.so" test
```

Resultado actual: **19 pruebas exitosas**, sin fallos, errores ni omitidas (nueve HTTP, nueve JNA y una de contexto). La nueva prueba verifica quantum persistido y recuperado internamente del historial, sin exponerlo en las respuestas públicas. Sin la propiedad se omiten las suites HTTP/JNA. Los ejemplos se regeneran en `target/ejemplos-http.json`. La compilación no mostró advertencias; el contexto conserva las advertencias conocidas de Mockito/Byte Buddy y agentes dinámicos.

Verificación externa completada el 2026-11-13: Keycloak real, PostgreSQL real, cuatro POST 201, aislamiento del historial y recuperación después de reiniciar Java. Ver [comandos, resultados y consulta visual de PostgreSQL](verificacion-real.md). No se modificaron Compose base, Angular, OpenAPI ni C; se agregó únicamente un override de red opcional para WSL NAT.
