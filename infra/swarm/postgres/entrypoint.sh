#!/bin/sh
set -eu
export KEYCLOAK_DB_PASSWORD="$(cat /run/secrets/keycloak_db_password)"
export PROCESO_DB_PASSWORD="$(cat /run/secrets/proceso_db_password)"
export PLANIFICADOR_DB_PASSWORD="$(cat /run/secrets/planificador_db_password)"
exec /usr/local/bin/docker-entrypoint.sh "$@"
