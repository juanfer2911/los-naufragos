#!/bin/sh
set -eu
export PLANIFICADOR_DB_PASSWORD="$(cat /run/secrets/planificador_db_password)"
exec java -jar /app/app.jar
