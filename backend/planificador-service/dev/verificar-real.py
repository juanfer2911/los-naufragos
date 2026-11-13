"""Ensayo local desde Windows: Keycloak real, HTTP y persistencia tras reinicio.

Lee infra/.env ignorado por Git. No imprime ni guarda contrasenas o tokens.
Crea usuarios de ensayo y, si falta, un cliente separado del cliente Angular.
El backend debe estar iniciado en WSL. Requiere Python 3, sin paquetes externos.
"""
import json
import secrets
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

SERVICIO = Path(__file__).resolve().parents[1]
RAIZ = SERVICIO.parents[1]
KC = "http://localhost:8080"
API = "http://localhost:8081"
REALM = "planificador-realm"
CLIENTE = "verificacion-backend-local"


def pedir(metodo, url, token=None, datos=None, formulario=False, esperado=200):
    headers = {}
    if token:
        headers["Authorization"] = "Bearer " + token
    if datos is None:
        cuerpo = None
    elif formulario:
        cuerpo = urllib.parse.urlencode(datos).encode()
        headers["Content-Type"] = "application/x-www-form-urlencoded"
    else:
        cuerpo = json.dumps(datos).encode()
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, data=cuerpo, headers=headers, method=metodo)
    try:
        respuesta = urllib.request.urlopen(req, timeout=30)
    except urllib.error.HTTPError as error:
        respuesta = error
    contenido = respuesta.read()
    if respuesta.status != esperado:
        raise RuntimeError(f"{metodo} {urllib.parse.urlparse(url).path}: HTTP {respuesta.status}, esperado {esperado}")
    return json.loads(contenido) if contenido else None


def ejecutar():
    env = dict(linea.split("=", 1) for linea in (RAIZ / "infra/.env").read_text().splitlines()
               if linea and not linea.startswith("#"))
    admin = pedir("POST", KC + "/realms/master/protocol/openid-connect/token", datos={
        "grant_type": "password", "client_id": "admin-cli",
        "username": env["KEYCLOAK_ADMIN_USERNAME"], "password": env["KEYCLOAK_ADMIN_PASSWORD"]
    }, formulario=True)["access_token"]
    ruta = KC + "/admin/realms/" + REALM
    try:
        pedir("GET", ruta, admin)
    except RuntimeError as error:
        if "HTTP 404," not in str(error):
            raise
        plantilla = json.loads((SERVICIO / "dev/keycloak-realm.json").read_text(encoding="utf-8"))
        pedir("POST", KC + "/admin/realms", admin, plantilla, esperado=201)
    clientes = pedir("GET", ruta + "/clients?clientId=" + CLIENTE, admin)
    if not clientes:
        pedir("POST", ruta + "/clients", admin, {
            "clientId": CLIENTE, "publicClient": True, "enabled": True,
            "standardFlowEnabled": False, "directAccessGrantsEnabled": True,
            "protocol": "openid-connect", "protocolMappers": [{
                "name": "audiencia-planificador", "protocol": "openid-connect",
                "protocolMapper": "oidc-audience-mapper", "config": {
                    "included.custom.audience": "planificador-service",
                    "access.token.claim": "true", "id.token.claim": "false"}}]
        }, esperado=201)

    def usuario():
        nombre = "verificacion-" + secrets.token_hex(4)
        clave = secrets.token_urlsafe(24)
        pedir("POST", ruta + "/users", admin, {
            "username": nombre, "enabled": True, "firstName": "Ensayo", "lastName": "Backend",
            "email": nombre + "@example.invalid", "emailVerified": True,
            "credentials": [{"type": "password", "value": clave, "temporary": False}]
        }, esperado=201)
        def autenticar():
            return pedir("POST", KC + "/realms/" + REALM + "/protocol/openid-connect/token", datos={
                "grant_type": "password", "client_id": CLIENTE, "username": nombre, "password": clave
            }, formulario=True)["access_token"]
        return autenticar

    autenticar = usuario()
    otro = usuario()()
    token = autenticar()
    pedir("GET", API + "/algoritmos", esperado=401)
    pedir("GET", API + "/algoritmos", "invalido", esperado=401)
    catalogo = pedir("GET", API + "/algoritmos", token)
    registros = []
    for algoritmo in catalogo["algorithms"]:
        entrada = {"algorithmId": algoritmo["id"], "quantum": 2, "processes": [
            {"pid": 1, "arrivalTime": 1, "burstTime": 5, "priority": 0},
            {"pid": 2, "arrivalTime": 2, "burstTime": 1, "priority": 0}]}
        registro = pedir("POST", API + "/planificaciones", token, entrada, esperado=201)
        assert len(registro["processMetrics"]) == 2 and registro["segments"]
        registros.append(registro)
        pedir("GET", API + "/historial/" + registro["planificationId"], otro, esperado=404)
    assert pedir("GET", API + "/historial", otro)["items"] == []
    assert len(pedir("GET", API + "/historial", token)["items"]) == len(registros)
    print("OK: Keycloak real, JWT, catalogo, cuatro POST 201, historial y aislamiento.", flush=True)
    print("Reinicia SOLO el backend y pulsa Enter para comprobar persistencia.", flush=True)
    input()
    token = autenticar()
    recuperados = pedir("GET", API + "/historial", token)["items"]
    assert {r["planificationId"] for r in recuperados} == {r["planificationId"] for r in registros}
    for registro in registros:
        assert pedir("GET", API + "/historial/" + registro["planificationId"], token) == registro
    informe = {"entorno": "PostgreSQL 16.4 + Keycloak 25.0.6 reales; Java 21 WSL + C",
               "persistencia_tras_reinicio": True, "catalogo": catalogo, "registros": registros}
    (SERVICIO / "target/verificacion-real.json").write_text(json.dumps(informe, indent=2), encoding="utf-8")
    print("OK: las planificaciones permanecen despues de reiniciar Java.", flush=True)
    print("Informe sin secretos: target/verificacion-real.json", flush=True)


if __name__ == "__main__":
    try:
        ejecutar()
    except Exception as error:
        print("No se completo el ensayo:", str(error), file=sys.stderr)
        sys.exit(1)
