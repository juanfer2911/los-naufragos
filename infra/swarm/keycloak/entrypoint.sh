#!/bin/sh
set -eu
export KEYCLOAK_ADMIN="$(cat /run/secrets/keycloak_admin_username)"
export KEYCLOAK_ADMIN_PASSWORD="$(cat /run/secrets/keycloak_admin_password)"
export KC_DB_PASSWORD="$(cat /run/secrets/keycloak_db_password)"
exec /opt/keycloak/bin/kc.sh "$@"
