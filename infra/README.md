# Infraestructura local

`docker-compose.yml` inicia únicamente PostgreSQL 16.4 y Keycloak 25.0.6. Keycloak usa PostgreSQL; la inicialización crea bases de datos y roles separados para Keycloak, `proceso-service` y `planificador-service`. El volumen `postgres_data` conserva los datos al detener los contenedores.

Esta configuración es para desarrollo e integración local. Todavía no incluye Angular ni los microservicios, no importa el realm `planificador-realm` y no es el manifiesto final para Docker Swarm.
El script SQL se ejecuta únicamente al inicializar un volumen de PostgreSQL vacío; los cambios posteriores a ese script no alteran automáticamente una base ya inicializada.

## Preparación

Desde la raíz del repositorio, copia la plantilla ignorada por Git y cambia los valores de ejemplo por credenciales locales:

```powershell
Copy-Item infra\.env.example infra\.env
notepad infra\.env
```

No guardes credenciales reales en `.env.example` ni en el repositorio. `infra/.env` está excluido por `.gitignore`.

## Iniciar

```powershell
docker compose --env-file infra/.env -f infra/docker-compose.yml up -d
```

## Comprobar

Verifica el estado de los servicios y que PostgreSQL acepte conexiones:

```powershell
docker compose --env-file infra/.env -f infra/docker-compose.yml ps
docker compose --env-file infra/.env -f infra/docker-compose.yml exec postgres pg_isready
docker compose --env-file infra/.env -f infra/docker-compose.yml logs --tail 50 keycloak
```

Keycloak queda disponible localmente en `http://localhost:8080`. En esta etapa solo se crea el administrador local de desarrollo; no hay realm de aplicación importado.

## Detener

```powershell
docker compose --env-file infra/.env -f infra/docker-compose.yml down
```

Este comando detiene y elimina los contenedores, pero conserva el volumen de PostgreSQL. Docker Swarm es el objetivo de despliegue posterior; requerirá una configuración de Swarm revisada, incluidos secretos y dependencias de arranque.
