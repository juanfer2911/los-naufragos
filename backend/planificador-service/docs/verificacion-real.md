# Verificación con PostgreSQL y Keycloak reales — 2026-11-13

## Resultado del ensayo inicial

- Docker Desktop inició correctamente: motor Linux 29.7.2 y Compose v5.5.0.
- Las primeras descargas fallaron por resolución DNS del distribuidor de imágenes. Un reintento descargó las imágenes originales, sin sustituir sus versiones.
- Se levantaron `postgres:16.4` y `quay.io/keycloak/keycloak:25.0.6` con el Compose existente. PostgreSQL saludable; SQL confirmó PostgreSQL 16.4 real.
- Se crearon `infra/.env` y `backend/planificador-service/.env`, ambos ignorados por Git, con credenciales locales generadas. No se muestran ni se copian a esta documentación.
- No existían contenedores/volúmenes de Los Náufragos. Se creó `infra_postgres_data`; se conservaron sin cambios los volúmenes y contenedores de otros proyectos.
- Se creó el realm `planificador-realm` desde la plantilla existente. Se conservó su cliente Angular con Authorization Code + PKCE. Para el ensayo se añadió `verificacion-backend-local`, separado del frontend, con password grant y audiencia `planificador-service`, y dos usuarios nuevos de verificación. Las contraseñas de ensayo y tokens permanecieron solo en memoria.
- Java Temurin 21.0.13 en Ubuntu/WSL cargó `native/libplanificador.so` y conectó a PostgreSQL con `planificador_user`. El arranque creó la tabla e índice previstos.
- HTTP real: sin token o con token inválido, 401; login real en Keycloak; catálogo 200; cuatro planificaciones (FCFS, SJF, SRTF, RR con quantum 2), 201; historial propio 200; historial del segundo usuario vacío; detalle ajeno 404.
- SQL confirmó cuatro filas, una por algoritmo. Su propietario coincidió con el ID del usuario autenticado en Keycloak.
- Se detuvo únicamente la JVM del backend y se inició otra. Tras autenticar de nuevo al mismo usuario, historial y detalles devolvieron exactamente los mismos IDs y resultados.

La captura sin secretos está en [verificacion-real.json](verificacion-real.json). Son respuestas de servicios reales; las capturas anteriores `ejemplos-http.json` siguen identificadas como ensayos H2/JWKS local.

En esta etapa se ejecutó Maven `package -DskipTests` (BUILD SUCCESS) y el ensayo externo descrito, no se volvieron a ejecutar las 18 pruebas locales anteriores. No hubo cambios en el código Java de producción ni en los algoritmos C.

## Cierre de etapa y aplicación del quantum

La verificación de cierre repitió las 34 pruebas nativas y la suite completa de 19 pruebas Java: cero fallos, errores u omitidas. Incluye la prueba de persistencia y recuperación interna del quantum, sin cambios del JSON HTTP. Solo se observaron las advertencias conocidas de Mockito/Byte Buddy; no advertencias de compilación.

El primer `package` pasó las pruebas pero falló al reemplazar el artefacto `.jar.original` con la JVM anterior activa. Se detuvo únicamente esa JVM y se repitió `package -DskipTests`, que terminó con BUILD SUCCESS. Se inició el JAR actualizado con Java 21 en WSL y PostgreSQL real; los contenedores no se reiniciaron.

Con un nuevo usuario técnico de ensayo y el cliente local ya existente se verificaron JWT real, 401 sin token/token inválido, catálogo 200 e historial privado. Se creó **una sola planificación RR con quantum 3**, necesaria para comprobar el nuevo snapshot de producción. POST devolvió 201; historial y detalle devolvieron el mismo resultado público sin quantum. SQL confirmó quantum 3 y propietario igual al sub autenticado. Las huellas de id/propietario/fecha/resultado de las cuatro filas anteriores permanecieron iguales: no se borraron ni alteraron históricos. El total local quedó en cinco planificaciones.

La captura sin secretos es [verificacion-cierre.json](verificacion-cierre.json). El backend quedó ejecutándose en `http://localhost:8081`, con PostgreSQL y Keycloak activos. Este cierre no repitió el ensayo inicial de cuatro algoritmos contra PostgreSQL: esos algoritmos se verificaron de nuevo en las suites automatizadas y se conserva la evidencia del ensayo real anterior.

## Red utilizada: Windows + WSL NAT

Ubuntu está en modo NAT, sin integración Docker Desktop habilitada. Docker se utilizó desde PowerShell; Java desde Ubuntu. Los puertos del Compose base están ligados a `127.0.0.1` de Windows, que no es el localhost de Ubuntu en este modo.

Se agregó `dev/compose.wsl.yaml`, con publicaciones adicionales solo sobre la IP virtual de Windows para WSL. No publica servicios en `0.0.0.0` ni en la interfaz Wi-Fi. Conserva el volumen y los puertos localhost originales. Requiere Compose con soporte de `!override` (verificado con v5.5.0).

Desde PowerShell, raíz del repositorio:

```powershell
# Si Desktop aún no está iniciado, abrirlo primero y esperar al motor Linux.
docker version
docker compose version
$env:WSL_HOST_IP = ((wsl -d Ubuntu -- ip route show default) -split '\s+')[2]
docker compose --env-file infra/.env -f infra/docker-compose.yml `
  -f backend/planificador-service/dev/compose.wsl.yaml up -d postgres keycloak
docker compose --env-file infra/.env -f infra/docker-compose.yml ps
```

La IP observada fue `172.23.160.1`; puede cambiar al reiniciar WSL. Confirmar que corresponde a `vEthernet (WSL)` con `Get-NetIPAddress`, no a Wi-Fi. Si cambia, repetir el comando con la IP nueva y actualizar **solo** las direcciones locales del `.env` del microservicio:

```text
PLANIFICADOR_DB_URL=jdbc:postgresql://172.23.160.1:5432/planificador_db
KEYCLOAK_JWK_SET_URI=http://172.23.160.1:8080/realms/planificador-realm/protocol/openid-connect/certs
KEYCLOAK_ISSUER_URI=http://localhost:8080/realms/planificador-realm
```

El emisor se mantiene como localhost porque el login se realiza desde Windows mediante esa URL. JWKS usa la dirección que Java alcanza desde WSL. No se desactivó la validación de emisor ni de audiencia para resolver la red.

Si WSL tiene red reflejada o localhost accesible, el override no es necesario: usar el Compose base y las direcciones localhost. No cambiar el modo de red ni reiniciar WSL para este ensayo.

## Repetir la ejecución del backend

Para actualizar una instancia ya abierta, detener primero únicamente su JVM (Ctrl+C si está en primer plano). No recompilar el mismo JAR mientras sigue ejecutándose: el cierre de etapa detectó un fallo de reemplazo de `.jar.original` al empaquetar y lo resolvió al detener la instancia anterior. No es necesario reiniciar PostgreSQL ni Keycloak. Para verificar y empaquetar juntos, usar `mvn ... -Dplanificador.biblioteca="$PWD/native/libplanificador.so" package`; el ejemplo con `-DskipTests` de abajo sirve cuando las pruebas ya pasaron.

Desde la raíz en Ubuntu, con JDK 21/Maven Linux configurados según el README:

```sh
export JAVA_HOME="$PWD/backend/planificador-service/target/herramientas/jdk-21.0.13+9"
export PATH="$JAVA_HOME/bin:$PWD/backend/planificador-service/target/herramientas/apache-maven-3.9.11/bin:$PATH"
mvn -f backend/planificador-service/pom.xml -DskipTests package
set -a
. backend/planificador-service/.env
set +a
java -jar backend/planificador-service/target/planificador-service-0.0.1-SNAPSHOT.jar
```

Usar `planificador_user` y la contraseña local que coincide con `infra/.env`. Nunca usar H2 para este arranque. El `.so` permanece en `native/`, sin copias. Si Java ya está ejecutándose en 8081, detener esa instancia antes de iniciar otra.

Durante esta sesión el backend quedó en segundo plano; el PID está en `target/backend-real.pid` y el log en `target/backend-real.log`. Antes de detenerlo, comprobar en WSL con `ps -p <PID> -o args=` que sigue siendo este JAR; no detener procesos por nombre genérico.

En otra terminal PowerShell, ejecutar el ensayo:

```powershell
python backend/planificador-service/dev/verificar-real.py
```

El script usa solo la biblioteca estándar de Python y las credenciales locales del admin de Keycloak. No sobrescribe realms/clientes existentes. Crea dos usuarios nuevos por ensayo, conserva sus datos y añade un cliente exclusivo de verificación si falta. Este cliente de password grant sirve únicamente para esta comprobación local; Angular conserva su flujo PKCE y su cliente independiente.

Cuando el script lo indique, detener el backend en su terminal con Ctrl+C, iniciarlo otra vez con el mismo comando y esperar al mensaje de arranque. Pulsar Enter en la terminal del ensayo. El script renueva el token, compara los registros antes/después y escribe `target/verificacion-real.json` sin secretos. No borra los datos de prueba. Las contraseñas de estos usuarios no se guardan; si se necesita usarlos manualmente, establecer una contraseña nueva desde la consola de Keycloak.

## Ver los datos en PostgreSQL

En la ventana **Database** de IntelliJ, añadir un origen PostgreSQL:

- Host: `localhost` (IntelliJ se ejecuta en Windows).
- Puerto: `5432`.
- Base: `planificador_db`.
- Usuario: `planificador_user`.
- Contraseña: la de `PLANIFICADOR_DB_PASSWORD` en el `.env` local, sin copiarla a archivos del repositorio.

Probar la conexión y abrir **Schemas → public → Tables → planificaciones**. Refrescar para ver las cuatro ejecuciones verificadas. Las columnas son `id`, `propietario`, `creado_en` y `resultado` (snapshot JSON). En la consola SQL:

```sql
SELECT id, propietario, creado_en,
       resultado::jsonb ->> 'algorithmId' AS algoritmo,
       resultado::jsonb -> 'averages' AS promedios
FROM public.planificaciones
ORDER BY creado_en DESC;
```

Para consultar desde PowerShell sin instalar otro programa:

```powershell
'SELECT id, propietario, creado_en, resultado::jsonb ->> ''algorithmId'' AS algoritmo FROM planificaciones ORDER BY creado_en;' |
  docker compose --env-file infra/.env -f infra/docker-compose.yml exec -T postgres `
    psql -U local_postgres_admin -d planificador_db
```

`local_postgres_admin` es el usuario local creado en esta sesión; si se reutiliza otra configuración, usar su `POSTGRES_ADMIN_USER`. El SQL es solo de lectura. El `propietario` puede contrastarse con el ID del usuario en la consola de Keycloak; no representa una cuenta local del backend.

## Lo que queda pendiente

No se verificó todavía el login de Angular ni su integración visual; no se modificó frontend. El catálogo, la ejecución, JWT, PostgreSQL y persistencia tras reiniciar Java quedaron comprobados. `proceso-service` y `/comparaciones` siguen fuera de esta etapa. No se añadieron pruebas extremas, clases de producción ni cambios al contrato compartido.
