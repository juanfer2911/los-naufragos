# proceso-service

## Quantum opcional por conjunto (2026-11-16)

`ProcessSetInput` y `ProcessSet` admiten ahora `quantum` opcional, entero mayor o igual que 1. POST lo guarda y GET lista/detalle lo devuelven cuando existe. No se extrae de la descripción ni cambia la ejecución del planificador. Ejemplo de entrada: `{"name":"Referencia RR","quantum":8,"processes":[{"pid":1,"arrivalTime":0,"burstTime":5,"priority":0}]}`.

Al arrancar, `schema.sql` ejecuta `ALTER TABLE conjuntos ADD COLUMN IF NOT EXISTS quantum BIGINT CHECK (quantum >= 1)`. La columna admite NULL: conserva las filas antiguas, sin reescribirlas; las respuestas antiguas omiten el campo. Angular carga 3 si no hay quantum y permite editarlo antes de ejecutar. No se modificó el script de creación de bases ni se añadieron dependencias de migración.

Verificación: siete pruebas HTTP de proceso-service exitosas (H2/JWT firmado), incluidos quantum persistido, valores inválidos y compatibilidad sin campo; catorce pruebas Angular y build exitosos. En Edge con PostgreSQL y Keycloak reales: crear con 8, logout/login y recarga recuperan 8; estudiante carga, cambia a 5 y ejecuta RR; conjunto antiguo carga 3; estudiante POST/DELETE 403. SQL confirmó quantum 8 y los datos anteriores intactos. Se reinició únicamente proceso-service para aplicar la ampliación; C y Keycloak no cambiaron. Evidencia local: `frontend/.angular/evidencias-quantum/verificacion.json`.

Java 21, Spring Boot 3.5.7, JDBC y PostgreSQL. Administra conjuntos compartidos conforme a `docs/openapi.yaml`. Servicio independiente en **8082**, sin llamadas a planificador-service ni dependencia de C. Angular debe enviar los procesos completos al planificador.

| Endpoint | Acceso | Resultado |
| --- | --- | --- |
| GET /conjuntos | Cualquier JWT válido | 200, `{ "items": [...] }` |
| POST /conjuntos | Rol de realm `docente` | 201, conjunto creado |
| GET /conjuntos/{conjuntoId} | Cualquier JWT válido | 200 o 404 |
| DELETE /conjuntos/{conjuntoId} | Rol de realm `docente` | 204 sin cuerpo o 404 |

Sin JWT válido: 401. Escritura sin rol docente: 403. POST inválido: 400; conflicto de integridad: 409; fallo inesperado: 500. Los errores tienen `code` y `message`; no exponen SQL ni credenciales. Los IDs son UUID canónicos; un ID inválido en GET/DELETE devuelve 404 (esas rutas no definen 400). No se exige unicidad del nombre. Cualquier docente puede eliminar un conjunto compartido.

El JSON de conjunto contiene `conjuntoId`, `name`, `description` opcional, `processes` y `quantum` opcional positivo. Se validan nombre no vacío de hasta 200 caracteres, descripción de hasta 2000, lista no vacía, procesos no nulos, PID entero no negativo y único, llegada entera no negativa, ráfaga entera positiva y prioridad entera obligatoria (admite negativos). Los tipos coinciden con el servicio existente: PID/prioridad de 32 bits y tiempos/quantum de 64 bits. Se rechazan números fraccionarios.

## Base y seguridad

La infraestructura existente crea `proceso_db`, propiedad de `proceso_user`. Al arrancar, `schema.sql` crea si faltan:

- `conjuntos`: UUID, nombre, descripción y `creador_sub` obtenido exclusivamente del JWT.
- `procesos`: referencia al conjunto, posición original, PID, llegada, ráfaga y prioridad. PK conjunto/PID, posición única por conjunto y borrado en cascada.

El conjunto y todos sus procesos se insertan en una transacción; un fallo revierte todo. Las consultas conservan el orden de los procesos y usan una transacción de lectura consistente. No hay tabla de usuarios, contraseñas, edición ni conjuntos personales. El SQL de inicio no elimina ni migra datos existentes.

Se valida firma RSA con JWKS, emisor, expiración, `sub` no vacío de hasta 255 caracteres y audiencia `proceso-service`. El rol se lee de **`realm_access.roles`**, valor exacto `docente`, y se convierte en `ROLE_docente` para Spring Security. No se acepta un rol enviado en el cuerpo ni un rol de otro cliente. Los estudiantes y usuarios sin rol docente pueden consultar.

En Keycloak, dentro de `planificador-realm`, los roles `estudiante` y `docente` ya existen. `estudiante` es predeterminado mediante `default-roles-planificador-realm`; `docente` se asigna únicamente por un administrador desde **Users → usuario → Role mapping → Assign role**, seleccionando el rol de realm `docente`. Nunca agregar `docente` en **Realm settings → Default roles**. Obtener un access token nuevo tras cambiar roles. La [plantilla del realm](../planificador-service/dev/keycloak-realm.json) conserva esta configuración para despliegues nuevos; no reimportarla sobre el realm existente.

El cliente que obtiene tokens debe incluir el scope `roles` y un mapper de audiencia `proceso-service` en el access token. Para Angular se puede añadir ese mapper en Keycloak conservando el mapper existente de `planificador-service`; no se necesita cambiar Angular para emitir ambas audiencias. Esta implementación no modifica el cliente Angular. Usar access tokens, no ID tokens.

El 2026-11-16 se verificó con una cuenta nueva: `estudiante` automático, sin `docente`, GET conjuntos 200, POST planificación 201 y POST/DELETE conjuntos 403. No se modificaron cuentas existentes ni sus contraseñas. Se conservaron la cuenta de ensayo y una planificación nueva en su historial; los conjuntos existentes quedaron intactos. Evidencia local sin secretos: `target/verificacion-roles-default.json`.

## Ejecutar desde IntelliJ o terminal

1. Iniciar PostgreSQL y Keycloak con la infraestructura existente, sin eliminar volúmenes.
2. Abrir `pom.xml` como proyecto Maven y seleccionar JDK 21.
3. Copiar `.env.example` a `.env` y configurar la contraseña de `proceso_user` con el valor local de `PROCESO_DB_PASSWORD` en `infra/.env`. No reutilizar la contraseña de planificador_user.
4. En la configuración de ejecución de `ProcesoServiceApplication`, cargar ese archivo como variables de entorno (o introducirlas manualmente). Spring Boot no carga `.env` automáticamente.
5. Ejecutar `ProcesoServiceApplication`. No requiere WSL ni biblioteca nativa.

Desde PowerShell, para cargar el archivo local y arrancar con Maven disponible en PATH:

```powershell
Get-Content backend/proceso-service/.env | ForEach-Object {
    if ($_ -match '^([A-Z_]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
mvn -f backend/proceso-service/pom.xml spring-boot:run
```

`SERVER_PORT` permite cambiar 8082. `PROCESO_DB_URL`, `PROCESO_DB_USER`, `PROCESO_DB_PASSWORD`, `KEYCLOAK_ISSUER_URI`, `KEYCLOAK_JWK_SET_URI`, `KEYCLOAK_AUDIENCE` y `FRONTEND_ORIGINS` están documentados en la plantilla. CORS admite orígenes explícitos, por defecto localhost:4200, y GET/POST/DELETE/OPTIONS.

## Comprobar mediante HTTP

Con `$tokenDocente` y `$tokenEstudiante` obtenidos de Keycloak, en memoria:

```powershell
$base = 'http://localhost:8082/conjuntos'
$docente = @{ Authorization = "Bearer $tokenDocente" }
$estudiante = @{ Authorization = "Bearer $tokenEstudiante" }
$body = '{"name":"Referencia","processes":[{"pid":1,"arrivalTime":0,"burstTime":5,"priority":0}]}'
$creado = Invoke-RestMethod $base -Method Post -Headers $docente -ContentType 'application/json' -Body $body
Invoke-RestMethod $base -Headers $estudiante
Invoke-RestMethod "$base/$($creado.conjuntoId)" -Headers $estudiante
# Estas dos operaciones deben devolver 403:
Invoke-RestMethod $base -Method Post -Headers $estudiante -ContentType 'application/json' -Body $body
Invoke-RestMethod "$base/$($creado.conjuntoId)" -Method Delete -Headers $estudiante
# Elimina únicamente el conjunto que acabamos de crear:
Invoke-WebRequest "$base/$($creado.conjuntoId)" -Method Delete -Headers $docente
```

## Pruebas

```powershell
mvn -f backend/proceso-service/pom.xml test
mvn -f backend/proceso-service/pom.xml package
python backend/proceso-service/dev/verificar-real.py
```

La suite HTTP usa Tomcat real, H2 y JWT RSA con un JWKS local: no sustituye al decoder por un mock. Comprueba ciclo completo, roles, JWT inválidos, validaciones, consulta compartida, cascada y rollback después de insertar el primer proceso.

El script real requiere Docker, Python 3, Java 21, JAR empaquetado, credenciales locales en `infra/.env`, realm existente y puerto 8082 libre. Crea un cliente de ensayo separado y dos usuarios con contraseñas aleatorias que conserva solo en memoria; añade roles si faltan, sin alterar usuarios/clientes preexistentes. Arranca y reinicia únicamente su propia JVM, conserva un conjunto de ensayo y elimina otro creado por él. Verifica SQL, permisos y persistencia tras reinicio; escribe evidencia sin secretos en `target/verificacion-real.json` y detiene su JVM al terminar. Los usuarios y cliente de ensayo permanecen en Keycloak. No modifica planificador-service ni sus datos.

### Resultado del 2026-11-15

Se ejecutó `package` con seis pruebas HTTP exitosas, cero fallos, errores u omisiones. Incluyen rollback real en H2 tras insertar un primer proceso, rechazo de datos inválidos, JWT y roles, consulta sin rol docente y fallos SQL controlados para verificar 409/500 sin filtrar detalles al cliente.

La integración con PostgreSQL y Keycloak reales confirmó GET lista/detalle 200, POST docente 201, DELETE docente 204, estudiante GET 200 y POST/DELETE 403, 401, 400, 404, creador_sub correcto, dos procesos, cascada y recuperación idéntica tras reiniciar Java. Se conservaron un conjunto de ensayo (`307631cc-d7b9-4eaf-8089-592712285425`), dos usuarios técnicos y un cliente separado. No había conjuntos previos. Se detuvo la JVM de prueba; el puerto 8082 queda libre para IntelliJ. Los 409/500 se verificaron en H2, sin provocar fallos en la infraestructura real. No se volvieron a ejecutar las suites de Angular o C en esta etapa: sus archivos y planificador-service permanecieron sin cambios.
