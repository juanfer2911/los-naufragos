#!/bin/sh
set -eu
export PROCESO_DB_PASSWORD="$(cat /run/secrets/proceso_db_password)"
exec java -jar /app/app.jar
