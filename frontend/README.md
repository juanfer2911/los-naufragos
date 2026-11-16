# Frontend de Los Náufragos

## Quantum guardado por conjunto (2026-11-16)

En Configuración, establecer el quantum deseado (por ejemplo 8) antes de guardar el banco. El formulario de conjuntos utiliza ese valor como dato real opcional `quantum`, junto con sus procesos. Al cargarlo se restaura el quantum guardado; para conjuntos antiguos sin ese campo se utiliza 3, incluso si antes había otro valor en pantalla. El campo de Configuración sigue siendo editable: cambiarlo a 5 afecta la siguiente ejecución de RR y no modifica el conjunto ya guardado.

OpenAPI se amplió únicamente con `quantum` opcional positivo en el modelo de conjunto. proceso-service lo persiste en una columna nullable de PostgreSQL. El quantum inicial continúa en 3; el historial y los algoritmos no cambiaron. Los permisos siguen siendo docente para guardar/eliminar y cualquier autenticado para cargar.

Pruebas: 14 Angular, 7 de proceso-service y build exitosos. Ensayo real en Edge: docente guarda 8, cierra sesión y vuelve a entrar; carga procesos y 8, también después de recargar. Estudiante carga 8, lo cambia a 5 y ejecuta RR con resultados reales. Conjunto antiguo carga 3 y estudiante recibe 403 al intentar crear/eliminar. Solo se eliminó el banco nuevo del ensayo; los anteriores permanecieron intactos. Evidencia sin secretos: `.angular/evidencias-quantum/verificacion.json`.

## Conjuntos compartidos: integración del 2026-11-16

Desde **Conjuntos de Prueba**, cualquier usuario autenticado puede consultar y cargar un conjunto en Configuración. Seleccionar los algoritmos y el quantum allí; los procesos completos y `sourceSetId` se envían al planificador existente. Gantt, métricas e historial siguen procediendo del backend/C.

El docente ve **Crear Nuevo Banco** y **Eliminar** (con confirmación). El formulario guarda los procesos actuales de Configuración, nombre de hasta 200 caracteres y descripción opcional de hasta 2000. El estudiante no ve esos controles; aunque manipule la interfaz, el backend exige el rol `docente` y responde 403. El rol se obtiene de `realm_access.roles`, se actualiza al renovar el token y se limpia al terminar la sesión. `estudiante` sigue predeterminado y `docente` sigue siendo asignación administrativa.

Se añadió únicamente el mapper de audiencia `proceso-service` al cliente Keycloak `angular-desarrollo`, conservando el mapper del planificador, Authorization Code + PKCE S256 y password grant desactivado. La plantilla del realm reproduce ambas audiencias y los roles predeterminados; no reimportar el realm existente. El interceptor envía el JWT solo a las dos URLs de backend configuradas. CORS existente de proceso-service ya permite localhost:4200, con GET/POST/DELETE y sin comodines; no requirió cambios.

Pruebas de esta etapa: build y **12 pruebas Angular** exitosas. Edge headless con Playwright local, Keycloak, ambos microservicios, PostgreSQL y C reales verificó login docente, creación desde Angular (201), SQL del creador y cuatro procesos, recarga, logout, login estudiante, consulta compartida, controles ocultos, rechazo real de POST/DELETE (403), FCFS/SJF/SRTF/RR, `sourceSetId`, Gantt y métricas RR iguales a la respuesta, historial después de recargar y eliminación docente desde Angular (204). Snapshot SQL confirmó que los conjuntos previos permanecieron intactos. Sin errores JavaScript. Evidencia sin secretos: `.angular/evidencias-conjuntos/verificacion.json` (ignorada por Git).

Se conservaron las cuentas técnicas nuevas y sus planificaciones de ensayo. Se eliminaron exclusivamente los conjuntos creados por esta prueba, incluido el residuo de un ensayo cuyo selector de historial necesitó corregirse. Las cuentas y contraseñas existentes no se modificaron. Se comprobó persistencia tras recarga y cambio de sesión; no se reiniciaron los servicios durante esta prueba. Las advertencias previas de CSS y CommonJS de Keycloak permanecen.

Para comprobarlo personalmente: iniciar sesión con tu docente, configurar procesos, abrir Conjuntos de Prueba, crear y guardar un banco; cerrar sesión, entrar con tu estudiante, cargarlo y ejecutar RR con quantum 2. Volver al docente y eliminar únicamente ese banco. El conjunto original se conserva.

## Arranque desde las terminales de IntelliJ IDEA

Mantener PostgreSQL y Keycloak activos. Ejecutar desde la raíz del repositorio, en terminales separadas. No iniciar una segunda instancia si los puertos ya están ocupados.

**proceso-service, PowerShell, Java 21 y Maven local de IntelliJ:**

```powershell
Get-Content infra/.env -Encoding UTF8 | ForEach-Object {
    if ($_ -match '^PROCESO_DB_PASSWORD=(.*)$') { $env:PROCESO_DB_PASSWORD = $matches[1] }
}
$env:SERVER_PORT = '8082'
$env:PROCESO_DB_URL = 'jdbc:postgresql://localhost:5432/proceso_db'
$env:PROCESO_DB_USER = 'proceso_user'
$env:KEYCLOAK_AUDIENCE = 'proceso-service'
$env:KEYCLOAK_ISSUER_URI = 'http://localhost:8080/realms/planificador-realm'
$env:KEYCLOAK_JWK_SET_URI = 'http://localhost:8080/realms/planificador-realm/protocol/openid-connect/certs'
$env:FRONTEND_ORIGINS = 'http://localhost:4200'
& 'C:/Program Files/JetBrains/IntelliJ IDEA 2026.2.2/plugins/maven-plugin/lib/maven3/bin/mvn.cmd' -f backend/proceso-service/pom.xml spring-boot:run
```

También se puede ejecutar `ProcesoServiceApplication` con JDK 21 y esas mismas variables en su configuración de IntelliJ. La contraseña local debe coincidir con la del volumen PostgreSQL; cambiar `.env` no cambia contraseñas de una base ya inicializada.

**planificador-service, terminal Ubuntu/WSL de IntelliJ, con su configuración local existente:**

```sh
cd /mnt/c/Users/juanf/Documents/los-naufragos
export JAVA_HOME="$PWD/backend/planificador-service/target/herramientas/jdk-21.0.13+9"
export PATH="$JAVA_HOME/bin:$PWD/backend/planificador-service/target/herramientas/apache-maven-3.9.11/bin:$PATH"
set -a
. backend/planificador-service/.env
set +a
mvn -f backend/planificador-service/pom.xml -Dmaven.repo.local=/mnt/c/Users/juanf/.m2/repository spring-boot:run
```

Su `.env` debe conservar `SERVER_PORT=8081`, audiencia `planificador-service`, la ruta Linux de `native/libplanificador.so` y las direcciones de PostgreSQL/JWKS accesibles desde WSL. Las herramientas bajo `target` son temporales y se eliminan con clean; para uso habitual configurar JDK/Maven Linux fuera de target.

**Angular, PowerShell desde la raíz:**

```powershell
Set-Location frontend
$node = (Resolve-Path '.angular/tools/node-v24.15.0-win-x64/node.exe').Path
$npm = (Resolve-Path '.angular/tools/npm12/node_modules/npm/bin/npm-cli.js').Path
$env:PATH = "$(Split-Path $node);$env:PATH"
& $node $npm start -- --host localhost --port 4200
```

Para repetir las pruebas unitarias, sustituir la última línea por `& $node $npm test -- --watch=false`; para compilar, `& $node $npm run build`. Abrir **http://localhost:4200**. No guardar tokens ni contraseñas en el repositorio.

Angular 22 conserva las pantallas de Mila y consume los endpoints de `planificador-service` y `proceso-service`. Keycloak autentica al usuario; cálculos, segmentos y métricas proceden de C mediante Java/JNA. Se retiró el simulador local después de comprobar el flujo real.

## Iniciar la aplicación completa

1. Iniciar Docker Desktop y PostgreSQL/Keycloak según la [guía del backend](../backend/planificador-service/docs/ejecucion-rest.md). Para WSL NAT utilizar su override documentado, sin eliminar volúmenes.
2. Iniciar `planificador-service` en Ubuntu/WSL con JDK 21, las variables de su `.env` local y `PLANIFICADOR_BIBLIOTECA` apuntando a `native/libplanificador.so`. Backend: http://localhost:8081.
3. Iniciar `proceso-service` en 8082 con Java 21 y las variables de PostgreSQL/Keycloak indicadas debajo.
4. Desde `frontend/`, con **Node 24.15.0 y npm 12.0.2** en el PATH:

```sh
npm ci --no-audit --no-fund
npm run build
npm test -- --watch=false
npm start -- --host 127.0.0.1 --port 4200
```

Abrir **http://localhost:4200/**, pulsar **Acceder con Keycloak** e introducir la contraseña únicamente en Keycloak. Utilizar un usuario del realm `planificador-realm`. En Configuración seleccionar algoritmos, editar procesos y quantum, ejecutar y consultar Resultados e Historial. Cada algoritmo seleccionado genera un POST y una planificación persistida. Cerrar Sesión termina la sesión real de Keycloak.

El lockfile requiere Node `^22.22.3 || ^24.15.0 || >=26.0.0`; Node 24.13.0 no cumple. No se redujeron esas restricciones. `npm ci` respeta el lockfile; build: `dist/frontend/`.

## Crear una cuenta en desarrollo local

El realm local `planificador-realm` permite registro de usuarios (`registrationAllowed: true`). Desde Angular, pulsar **Acceder con Keycloak** y luego **Register / Registrarse**, debajo del formulario de Keycloak. Completar usuario, correo, contraseña y confirmación, nombre y apellido. La contraseña se introduce en Keycloak, nunca en Angular. Si ya hay una sesión abierta, cerrarla antes para ver el formulario de acceso.

Esta opción se habilitó únicamente en el Keycloak local y en su plantilla de desarrollo; no establece una política para producción. Para un realm local existente, consola administrativa → `planificador-realm` → **Realm settings → Login → User registration**, activar y guardar. No reimportar el realm sobre datos existentes.

Verificado desde el navegador: registro público, cierre e inicio de sesión posterior, JWT aceptado por el backend, catálogo 200, planificación FCFS 201 e historial inicialmente vacío y luego con una sola ejecución propia. El detalle de una planificación de otro usuario devolvió 404. No se modificó la seguridad del backend.

## Configuración pública de desarrollo

[app.settings.ts](src/app/app.settings.ts) contiene URLs e identificadores públicos, sin secretos. Abrir el navegador con `localhost`, el origen permitido.

- Keycloak: http://localhost:8080, realm `planificador-realm`, cliente público `angular-desarrollo`.
- Authorization Code con PKCE S256; redirect URI `http://localhost:4200/*` y Web Origin `http://localhost:4200`. Logout reutiliza las redirect URIs explícitas (`post.logout.redirect.uris: +`).
- Scopes por defecto: `basic`, `profile`, `email`, `roles`; `basic` incorpora `sub` en Keycloak 25. Mappers de audiencia `planificador-service` y `proceso-service` para el access token. Sin password grant ni secretos de cliente.
- La [plantilla del cliente](../backend/planificador-service/dev/keycloak-realm.json) prepara un realm nuevo. En uno existente ajustar el cliente, sin reimportar ni reemplazar usuarios/datos.
- `keycloak-js` 25.0.6 mantiene tokens en memoria, renueva el access token y utiliza check-sso al recargar. El interceptor agrega Bearer solamente al backend configurado. No se guardan tokens en localStorage/sessionStorage ni archivos.
- CORS del backend ya permite el origen local configurado; no fue necesario ampliarlo.

## Contrato y límites

HttpClient consume `GET /algoritmos`, `POST /planificaciones`, `GET /historial` y `GET /historial/{planificacionId}` según OpenAPI, sin modificarlo. Los campos españoles se traducen a `pid`, `arrivalTime`, `burstTime`, `priority`; RR envía `quantum`. Gantt distingue `kind: "idle"` de `"execution"`, sin PID especial. Métricas y promedios vienen del backend.

Los conjuntos son compartidos y persistentes en PostgreSQL mediante proceso-service (8082). Cargar consulta el detalle, lleva sus procesos a la pantalla de Configuracion y conserva el sourceSetId real al planificar. No hay edicion de conjuntos ni fechas inventadas; el contrato no los define. El quantum historico no se publica: debe indicarse de nuevo para repetir RR.

## Verificación realizada

- Build correcto. Advertencias: CSS existente de 5.97 kB frente a aviso de 4 kB (bajo el límite de error de 8 kB) y CommonJS de `js-sha256`, dependencia transitiva de Keycloak. Se conservaron diseño y presupuestos.
- **7 pruebas Angular exitosas**: componente/acceso, mapeo de ocio y métricas, catálogo dinámico, POST con quantum, historial/detalle e interceptor que no filtra JWT a otros orígenes.
- **19 pruebas backend exitosas** en WSL con C real, H2 y JWKS firmado de prueba; no se presentan como pruebas PostgreSQL/Keycloak.
- Además, Edge contra PostgreSQL y Keycloak reales comprobó PKCE S256, catálogo, cuatro POST 201, métricas visibles iguales a las respuestas, ocio, historial/detalle después de recargar con SSO, CORS y logout, sin errores JavaScript. SQL confirmó cuatro filas asociadas al `sub` de ensayo y quantum 2 para RR. No se borraron registros previos.
- El ensayo detectó que faltaba `basic` en el cliente Angular: se añadió al cliente existente y a la plantilla sin relajar la seguridad del backend.

Capturas e informes locales: `.angular/evidencias-real/`, ignorados por Git, sin contraseñas ni tokens. Las cuentas de ensayo usaron contraseñas aleatorias solo en memoria.

## Herramientas locales de este equipo

Node y npm descargados bajo `.angular/tools/` están ignorados. Desde PowerShell en `frontend/`:

```powershell
$node = (Resolve-Path '.angular/tools/node-v24.15.0-win-x64/node.exe').Path
$npm = (Resolve-Path '.angular/tools/npm12/node_modules/npm/bin/npm-cli.js').Path
$env:PATH = "$(Split-Path $node);$env:PATH"
& $node $npm ci --no-audit --no-fund
& $node $npm start -- --host 127.0.0.1 --port 4200
```

En otro equipo instalar versiones compatibles en el PATH. No versionar `.angular/`, `node_modules/`, `dist/`, logs, contraseñas o tokens. La persistencia de conjuntos ya esta integrada; el despliegue final sigue pendiente.
