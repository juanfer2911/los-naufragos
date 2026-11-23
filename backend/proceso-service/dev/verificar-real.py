"""Ensayo local PostgreSQL/Keycloak. Crea solo datos propios de ensayo.

Lee infra/.env; no imprime ni guarda secretos. Conserva un conjunto para
demostrar persistencia y elimina otro para verificar DELETE y cascada.
No modifica clientes, usuarios ni conjuntos preexistentes.
"""
import json
import os
from pathlib import Path
import secrets
import subprocess
import time
import urllib.error
import urllib.parse
import urllib.request

SERVICIO = Path(__file__).resolve().parents[1]
RAIZ = SERVICIO.parents[1]
KC = "http://localhost:8080"
API = "http://localhost:8082/conjuntos"
REALM = "planificador-realm"


def pedir(metodo, url, token=None, datos=None, formulario=False, esperado=200):
    headers = {}
    if token:
        headers["Authorization"] = "Bearer " + token
    cuerpo = None
    if datos is not None:
        cuerpo = (urllib.parse.urlencode(datos) if formulario else json.dumps(datos)).encode()
        headers["Content-Type"] = "application/x-www-form-urlencoded" if formulario else "application/json"
    req = urllib.request.Request(url, data=cuerpo, headers=headers, method=metodo)
    try:
        respuesta = urllib.request.urlopen(req, timeout=30)
    except urllib.error.HTTPError as error:
        respuesta = error
    with respuesta:
        contenido = respuesta.read()
        if respuesta.status != esperado:
            raise RuntimeError(f"{metodo} {urllib.parse.urlparse(url).path}: HTTP {respuesta.status}, esperado {esperado}")
    return json.loads(contenido) if contenido else None


def ejecutar():
    env = dict(line.split("=", 1) for line in (RAIZ / "infra/.env").read_text().splitlines()
               if line and not line.startswith("#") and "=" in line)
    admin = pedir("POST", KC + "/realms/master/protocol/openid-connect/token", datos={
        "grant_type": "password", "client_id": "admin-cli",
        "username": env["KEYCLOAK_ADMIN_USERNAME"], "password": env["KEYCLOAK_ADMIN_PASSWORD"]
    }, formulario=True)["access_token"]
    ruta = KC + "/admin/realms/" + REALM
    roles = pedir("GET", ruta + "/roles", admin)
    for nombre in ("docente", "estudiante"):
        if not any(r["name"] == nombre for r in roles):
            pedir("POST", ruta + "/roles", admin, {"name": nombre}, esperado=201)
    cliente = "verificacion-proceso-" + secrets.token_hex(4)
    pedir("POST", ruta + "/clients", admin, {
        "clientId": cliente, "publicClient": True, "enabled": True,
        "standardFlowEnabled": False, "directAccessGrantsEnabled": True,
        "protocol": "openid-connect", "defaultClientScopes": ["basic", "roles"],
        "protocolMappers": [{"name": "audiencia-proceso", "protocol": "openid-connect",
            "protocolMapper": "oidc-audience-mapper", "config": {
                "included.custom.audience": "proceso-service", "access.token.claim": "true",
                "id.token.claim": "false"}}]}, esperado=201)

    def usuario(rol):
        nombre = "ensayo-proceso-" + secrets.token_hex(4)
        clave = secrets.token_urlsafe(24)
        pedir("POST", ruta + "/users", admin, {
            "username": nombre, "enabled": True, "firstName": "Ensayo", "lastName": "Proceso",
            "email": nombre + "@example.invalid", "emailVerified": True,
            "credentials": [{"type": "password", "value": clave, "temporary": False}]}, esperado=201)
        uid = pedir("GET", ruta + "/users?username=" + nombre, admin)[0]["id"]
        role = pedir("GET", ruta + "/roles/" + rol, admin)
        pedir("POST", ruta + "/users/" + uid + "/role-mappings/realm", admin, [role], esperado=204)
        token = pedir("POST", KC + "/realms/" + REALM + "/protocol/openid-connect/token", datos={
            "grant_type": "password", "client_id": cliente, "username": nombre, "password": clave
        }, formulario=True)["access_token"]
        return uid, token

    docente_id, docente = usuario("docente")
    _, estudiante = usuario("estudiante")
    entorno = os.environ.copy()
    entorno.update(PROCESO_DB_PASSWORD=env["PROCESO_DB_PASSWORD"], SERVER_PORT="8082",
                   KEYCLOAK_ISSUER_URI=KC + "/realms/" + REALM,
                   KEYCLOAK_JWK_SET_URI=KC + "/realms/" + REALM + "/protocol/openid-connect/certs",
                   KEYCLOAK_AUDIENCE="proceso-service")
    jar = SERVICIO / "target/proceso-service-0.0.1-SNAPSHOT.jar"
    log = open(SERVICIO / "target/servicio-real.log", "w")

    def iniciar():
        p = subprocess.Popen(["java", "-jar", str(jar)], env=entorno, stdout=log, stderr=log)
        for _ in range(120):
            if p.poll() is not None:
                raise RuntimeError("Java no inicio; revisar target/servicio-real.log")
            try:
                pedir("GET", API, esperado=401)
                return p
            except (OSError, RuntimeError):
                time.sleep(1)
        p.terminate()
        raise RuntimeError("Tiempo de arranque agotado")

    def sql(consulta):
        return subprocess.check_output(["docker", "exec", "infra-postgres-1", "psql", "-U",
            env["POSTGRES_ADMIN_USER"], "-d", "proceso_db", "-Atc", consulta], text=True).strip()

    p = iniciar()
    try:
        anteriores = pedir("GET", API, estudiante)["items"]
        pedir("GET", API, "invalido", esperado=401)
        entrada = {"name": "Ensayo persistencia " + secrets.token_hex(4), "processes": [
            {"pid": 2, "arrivalTime": 0, "burstTime": 5, "priority": -1},
            {"pid": 1, "arrivalTime": 2, "burstTime": 3, "priority": 0}]}
        pedir("POST", API, estudiante, entrada, esperado=403)
        registro = pedir("POST", API, docente, entrada, esperado=201)
        id = registro["conjuntoId"]
        assert pedir("GET", API + "/" + id, estudiante) == registro
        pedir("DELETE", API + "/" + id, estudiante, esperado=403)
        assert sql(f"SELECT creador_sub FROM conjuntos WHERE conjunto_id='{id}'") == docente_id
        assert sql(f"SELECT count(*) FROM procesos WHERE conjunto_id='{id}'") == "2"
        pedir("POST", API, docente, {"name": "Invalido", "processes": []}, esperado=400)
        pedir("GET", API + "/no-uuid", estudiante, esperado=404)
        borrar = pedir("POST", API, docente, entrada, esperado=201)["conjuntoId"]
        pedir("DELETE", API + "/" + borrar, docente, esperado=204)
        pedir("DELETE", API + "/" + borrar, docente, esperado=404)
        assert sql(f"SELECT count(*) FROM procesos WHERE conjunto_id='{borrar}'") == "0"
        p.terminate()
        p.wait(timeout=30)
        p = iniciar()
        assert pedir("GET", API + "/" + id, estudiante) == registro
        actuales = pedir("GET", API, estudiante)["items"]
        for anterior in anteriores:
            assert anterior in actuales
        informe = {"entorno": "Java 21 Windows, PostgreSQL y Keycloak reales",
                   "persistencia_tras_reinicio": True, "conjuntos_previos_intactos": len(anteriores),
                   "conjunto_conservado": id, "roles": ["docente", "estudiante"],
                   "verificado": ["GET lista/detalle 200", "POST docente 201", "DELETE docente 204",
                                  "estudiante POST/DELETE 403", "JWT ausente/invalido 401",
                                  "validacion 400", "inexistente 404", "SQL sub/procesos/cascada"]}
        (SERVICIO / "target/verificacion-real.json").write_text(json.dumps(informe, indent=2))
        print(json.dumps(informe, indent=2))
    finally:
        p.terminate()
        p.wait(timeout=30)
        log.close()


if __name__ == "__main__":
    ejecutar()
