# Frontend de Los Náufragos

Angular 22 conserva las pantallas de Mila y consume los cuatro endpoints de `planificador-service`. Keycloak autentica al usuario; cálculos, segmentos y métricas proceden de C mediante Java/JNA. Se retiró el simulador local después de comprobar el flujo real.

## Iniciar la aplicación completa

1. Iniciar Docker Desktop y PostgreSQL/Keycloak según la [guía del backend](../backend/planificador-service/docs/ejecucion-rest.md). Para WSL NAT utilizar su override documentado, sin eliminar volúmenes.
2. Iniciar `planificador-service` en Ubuntu/WSL con JDK 21, las variables de su `.env` local y `PLANIFICADOR_BIBLIOTECA` apuntando a `native/libplanificador.so`. Backend: http://localhost:8081.
3. Desde `frontend/`, con **Node 24.15.0 y npm 12.0.2** en el PATH:

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
- Scopes por defecto: `basic`, `profile`, `email`, `roles`; `basic` incorpora `sub` en Keycloak 25. Mapper de audiencia `planificador-service` para el access token. Sin password grant ni secretos de cliente.
- La [plantilla del cliente](../backend/planificador-service/dev/keycloak-realm.json) prepara un realm nuevo. En uno existente ajustar el cliente, sin reimportar ni reemplazar usuarios/datos.
- `keycloak-js` 25.0.6 mantiene tokens en memoria, renueva el access token y utiliza check-sso al recargar. El interceptor agrega Bearer solamente al backend configurado. No se guardan tokens en localStorage/sessionStorage ni archivos.
- CORS del backend ya permite el origen local configurado; no fue necesario ampliarlo.

## Contrato y límites

HttpClient consume `GET /algoritmos`, `POST /planificaciones`, `GET /historial` y `GET /historial/{planificacionId}` según OpenAPI, sin modificarlo. Los campos españoles se traducen a `pid`, `arrivalTime`, `burstTime`, `priority`; RR envía `quantum`. Gantt distingue `kind: "idle"` de `"execution"`, sin PID especial. Métricas y promedios vienen del backend.

Los conjuntos son **provisionales y locales en memoria**, identificados así en la pantalla: se pierden al recargar y no se envía un `sourceSetId` ficticio. Su persistencia espera a `proceso-service`. El quantum histórico se conserva internamente en PostgreSQL, pero OpenAPI no lo publica: al abrir una corrida RR hay que introducirlo para volver a ejecutarla. Exportar PDF utiliza la impresión del navegador.

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

En otro equipo instalar versiones compatibles en el PATH. No versionar `.angular/`, `node_modules/`, `dist/`, logs, contraseñas o tokens. Quedan pendientes `proceso-service`, persistencia de conjuntos y despliegue final.
