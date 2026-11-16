# Infraestructura local y Docker Swarm

`docker-compose.yml` mantiene el entorno local actual con PostgreSQL 16.4 y Keycloak 25.0.6. Conserva sus datos en el volumen `postgres_data`.

`swarm/stack.yml` agrega Angular, los dos microservicios, PostgreSQL y Keycloak, e importa el realm `planificador-realm`. Es un despliegue local de demostración: usa Keycloak 25.0.6 con configuración preconstruida y HTTP, publica los puertos de desarrollo y no incluye TLS ni respaldos. No lo expongas directamente a Internet.

El realm limita los access tokens a 60 segundos; Angular intenta renovarlos cuando quedan menos de 30 segundos.

El script SQL crea las bases y los roles solo cuando PostgreSQL inicia con un volumen vacío. El stack Swarm usa un volumen independiente llamado `naufragos_postgres_data`; no comparte ni borra los datos del Compose actual. Conserva ese volumen al retirar el stack.

## Preparación

Si aún no tienes `infra/.env`, copia la plantilla ignorada por Git y cambia los valores de ejemplo:

```powershell
Copy-Item infra\.env.example infra\.env
notepad infra\.env
```

No guardes credenciales reales en `.env.example` ni en el repositorio.

## Iniciar servicios locales con Compose

```powershell
docker compose --env-file infra/.env -f infra/docker-compose.yml up -d
```

Keycloak queda disponible localmente en `http://localhost:8080`.

## Iniciar el stack Docker Swarm local

En el primer despliegue, el script comprueba credenciales y los puertos `4200`, `8080`, `8081` y `8082`; si uno está ocupado, se detiene sin cerrar procesos. Al actualizar un stack ya creado, permite los puertos que ese propio stack publica. Luego crea secretos Docker desde `infra/.env`, construye las cinco imágenes y despliega:

```powershell
.\infra\swarm\iniciar.ps1
```

Abre `http://localhost:4200`. Los secretos quedan en Docker al retirar el stack para permitir volver a iniciarlo. Si cambias las credenciales de `.env`, elimina los secretos `naufragos_*` con el stack detenido y vuelve a ejecutar el script.

Para revisar el estado:

```powershell
docker stack services naufragos
docker stack ps naufragos
docker service logs naufragos_planificador-service
```

## Detener el stack Swarm

```powershell
.\infra\swarm\detener.ps1
```

Retira los servicios y la red del stack, pero conserva la base y los secretos.

## Detener Compose local

```powershell
docker compose --env-file infra/.env -f infra/docker-compose.yml down
```

Este comando detiene y elimina los contenedores, pero conserva el volumen de PostgreSQL.
