package com.losnaufragos.planificador;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.nimbusds.jose.*;
import com.nimbusds.jose.crypto.RSASSASigner;
import com.nimbusds.jose.jwk.JWKSet;
import com.nimbusds.jose.jwk.RSAKey;
import com.nimbusds.jose.jwk.gen.RSAKeyGenerator;
import com.nimbusds.jwt.JWTClaimsSet;
import com.nimbusds.jwt.SignedJWT;
import com.sun.net.httpserver.HttpServer;
import org.junit.jupiter.api.*;
import org.junit.jupiter.api.condition.EnabledIfSystemProperty;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.test.web.server.LocalServerPort;
import org.springframework.http.*;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.time.Instant;
import java.util.*;
import static org.junit.jupiter.api.Assertions.*;

// HTTP real y C real; H2 y emisor/JWKS local de prueba, no PostgreSQL/Keycloak.
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@EnabledIfSystemProperty(named = "planificador.biblioteca", matches = ".+")
class ApiIntegracionTests {
    private static final RSAKey CLAVE;
    private static final HttpServer JWKS;
    private static final String ISSUER;
    static {
        try {
            CLAVE = new RSAKeyGenerator(2048).keyID("pruebas").generate();
            JWKS = HttpServer.create(new InetSocketAddress("127.0.0.1", 0), 0);
            JWKS.createContext("/certs", exchange -> {
                byte[] bytes = new JWKSet(CLAVE.toPublicJWK()).toString().getBytes(StandardCharsets.UTF_8);
                exchange.getResponseHeaders().set("Content-Type", "application/json");
                exchange.sendResponseHeaders(200, bytes.length);
                try (var salida = exchange.getResponseBody()) { salida.write(bytes); }
            });
            JWKS.start();
            ISSUER = "http://127.0.0.1:" + JWKS.getAddress().getPort();
        } catch (Exception e) { throw new ExceptionInInitializerError(e); }
    }

    @DynamicPropertySource
    static void propiedades(DynamicPropertyRegistry registro) {
        registro.add("spring.security.oauth2.resourceserver.jwt.issuer-uri", () -> ISSUER);
        registro.add("spring.security.oauth2.resourceserver.jwt.jwk-set-uri", () -> ISSUER + "/certs");
    }
    @AfterAll static void cerrar() { JWKS.stop(0); }

    @Autowired TestRestTemplate http;
    @Autowired ObjectMapper json;
    @Autowired JdbcClient jdbc;
    @Autowired PlanificacionRepositorio repositorio;
    @LocalServerPort int puerto;

    private String token(String sub) throws Exception {
        return token(sub, ISSUER, "planificador-service", Instant.now().plusSeconds(300));
    }
    private String token(String sub, String issuer, String audience, Instant fin) throws Exception {
        var claims = new JWTClaimsSet.Builder().subject(sub).issuer(issuer).audience(audience)
                .issueTime(Date.from(Instant.now())).expirationTime(Date.from(fin)).build();
        var jwt = new SignedJWT(new JWSHeader.Builder(JWSAlgorithm.RS256).keyID("pruebas").build(), claims);
        jwt.sign(new RSASSASigner(CLAVE));
        return jwt.serialize();
    }
    private ResponseEntity<JsonNode> solicitar(HttpMethod metodo, String ruta, String jwt, String cuerpo) {
        var headers = new HttpHeaders();
        headers.setContentType(MediaType.APPLICATION_JSON);
        if (jwt != null) headers.setBearerAuth(jwt);
        return http.exchange(ruta, metodo, new HttpEntity<>(cuerpo, headers), JsonNode.class);
    }
    private String entrada(String algoritmo) {
        return """
                {"algorithmId":"%s","processes":[
                {"pid":1,"arrivalTime":1,"burstTime":5,"priority":0},
                {"pid":2,"arrivalTime":2,"burstTime":1,"priority":0}],"quantum":2}
                """.formatted(algoritmo);
    }

    @Test void rechazaJwtAusenteYNoValido() throws Exception {
        assertEquals(401, solicitar(HttpMethod.GET, "/algoritmos", null, null).getStatusCode().value());
        String firmado = token("a");
        int inicioFirma = firmado.lastIndexOf('.') + 1;
        String alterado = firmado.substring(0, inicioFirma) + (firmado.charAt(inicioFirma) == 'A' ? 'B' : 'A')
                + firmado.substring(inicioFirma + 1);
        for (String valor : List.of("invalido", alterado, token("a", "http://emisor-ajeno", "planificador-service", Instant.now().plusSeconds(300)),
                token("a", ISSUER, "otra-audiencia", Instant.now().plusSeconds(300)),
                token("a", ISSUER, "planificador-service", Instant.now().minusSeconds(300)), token(""))) {
            var respuesta = solicitar(HttpMethod.GET, "/algoritmos", valor, null);
            assertEquals(401, respuesta.getStatusCode().value());
            assertEquals("UNAUTHORIZED", respuesta.getBody().path("code").asText());
        }
    }

    @ParameterizedTest @ValueSource(strings = {"fcfs", "sjf", "srtf", "rr"})
    void ejecutaYGuardaMedianteHttp(String algoritmo) throws Exception {
        String jwt = token("ejecutor-" + algoritmo);
        var respuesta = solicitar(HttpMethod.POST, "/planificaciones", jwt, entrada(algoritmo));
        assertEquals(201, respuesta.getStatusCode().value());
        var registro = respuesta.getBody();
        assertEquals(algoritmo, registro.path("algorithmId").asText());
        assertEquals("idle", registro.path("segments").get(0).path("kind").asText());
        assertFalse(registro.path("segments").get(0).has("pid"));
        assertEquals(2, registro.path("processMetrics").size());
        assertEquals(1, registro.path("processMetrics").get(0).path("arrivalTime").asLong());
        assertTrue(registro.path("averages").has("waitingTime"));
        String id = registro.path("planificationId").asText();
        assertEquals("ejecutor-" + algoritmo, jdbc.sql("SELECT propietario FROM planificaciones WHERE id = :id")
                .param("id", id).query(String.class).single());
        assertEquals(registro, solicitar(HttpMethod.GET, "/historial/" + id, jwt, null).getBody());
        if (algoritmo.equals("fcfs")) {
            assertEquals(2.0, registro.path("averages").path("waitingTime").asDouble());
            assertEquals(5.0, registro.path("averages").path("turnaroundTime").asDouble());
            var ejemplo = json.createObjectNode();
            ejemplo.put("entorno", "HTTP + JWT RSA/JWKS local + H2 + C real; sin Keycloak/PostgreSQL");
            ejemplo.set("request", json.readTree(entrada(algoritmo)));
            ejemplo.put("status", 201);
            ejemplo.set("response", registro);
            ejemplo.set("catalog", solicitar(HttpMethod.GET, "/algoritmos", jwt, null).getBody());
            ejemplo.set("history", solicitar(HttpMethod.GET, "/historial", jwt, null).getBody());
            ejemplo.set("unauthorized", solicitar(HttpMethod.GET, "/algoritmos", null, null).getBody());
            Files.writeString(Path.of("target/ejemplos-http.json"), json.writerWithDefaultPrettyPrinter().writeValueAsString(ejemplo));
        }
    }

    @Test void conservaQuantumEnHistorialSinCambiarRespuestaPublica() throws Exception {
        String sub = "historial-quantum";
        String jwt = token(sub);
        var respuesta = solicitar(HttpMethod.POST, "/planificaciones", jwt,
                entrada("rr").replace("\"quantum\":2", "\"quantum\":3"));
        assertEquals(201, respuesta.getStatusCode().value());
        var registro = respuesta.getBody();
        String id = registro.path("planificationId").asText();
        String snapshot = jdbc.sql("SELECT resultado FROM planificaciones WHERE id = :id")
                .param("id", id).query(String.class).single();
        assertEquals(3, json.readTree(snapshot).path("quantum").asLong());
        assertEquals(3L, repositorio.buscar(sub, id).orElseThrow().quantum());
        assertEquals(3L, repositorio.listar(sub).getFirst().quantum());
        assertFalse(registro.has("quantum"));
        assertEquals(registro, solicitar(HttpMethod.GET, "/historial/" + id, jwt, null).getBody());
        assertFalse(solicitar(HttpMethod.GET, "/historial", jwt, null).getBody().path("items").get(0).has("quantum"));
    }

    @Test void historialNoExponeOtroUsuario() throws Exception {
        String alice = token("propietaria-alice");
        String bob = token("propietario-bob");
        var registro = solicitar(HttpMethod.POST, "/planificaciones", alice,
                entrada("fcfs").replace("\"quantum\":2", "\"quantum\":2,\"userId\":\"propietario-bob\"")).getBody();
        assertEquals(404, solicitar(HttpMethod.GET, "/historial/" + registro.path("planificationId").asText(), bob, null).getStatusCode().value());
        assertTrue(solicitar(HttpMethod.GET, "/historial", bob, null).getBody().path("items").isEmpty());
        assertEquals(1, solicitar(HttpMethod.GET, "/historial", alice, null).getBody().path("items").size());
    }

    @Test void validaEntradasSinPersistir() throws Exception {
        int antes = jdbc.sql("SELECT COUNT(*) FROM planificaciones").query(Integer.class).single();
        for (String valor : List.of(entrada("inexistente"), entrada("rr").replace(",\"quantum\":2", ""),
                entrada("rr").replace("\"quantum\":2", "\"quantum\":0"), entrada("fcfs").replace("\"burstTime\":5", "\"burstTime\":0"),
                entrada("fcfs").replace("\"pid\":2", "\"pid\":1"), entrada("fcfs").replace("\"arrivalTime\":1", "\"arrivalTime\":1.5"))) {
            var respuesta = solicitar(HttpMethod.POST, "/planificaciones", token("invalido"), valor);
            assertEquals(400, respuesta.getStatusCode().value());
            assertEquals("BAD_REQUEST", respuesta.getBody().path("code").asText());
        }
        assertEquals(antes, jdbc.sql("SELECT COUNT(*) FROM planificaciones").query(Integer.class).single());
    }

    @Test void corsLimitaOrigenes() throws Exception {
        for (String origen : List.of("http://localhost:4200", "https://sitio-ajeno.example")) {
            var peticion = java.net.http.HttpRequest.newBuilder(java.net.URI.create("http://localhost:" + puerto + "/planificaciones"))
                    .header("Origin", origen).header("Access-Control-Request-Method", "POST")
                    .header("Access-Control-Request-Headers", "Authorization, Content-Type")
                    .method("OPTIONS", java.net.http.HttpRequest.BodyPublishers.noBody()).build();
            var respuesta = java.net.http.HttpClient.newHttpClient().send(peticion, java.net.http.HttpResponse.BodyHandlers.ofString());
            if (origen.equals("http://localhost:4200")) {
                assertEquals(200, respuesta.statusCode());
                assertEquals(origen, respuesta.headers().firstValue("Access-Control-Allow-Origin").orElse(null));
            } else assertEquals(403, respuesta.statusCode());
        }
    }
}
