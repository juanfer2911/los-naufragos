package com.losnaufragos.proceso;

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
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.http.*;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.*;
import static org.junit.jupiter.api.Assertions.*;

// HTTP real; H2 y emisor/JWKS local de prueba, no PostgreSQL/Keycloak.
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)

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


    private String token(String sub) throws Exception {
        return token(sub, ISSUER, "proceso-service", Instant.now().plusSeconds(300));
    }
    private String token(String sub, String issuer, String audience, Instant fin) throws Exception {
        var claims = new JWTClaimsSet.Builder().subject(sub).issuer(issuer).audience(audience)
                .claim("realm_access", Map.of("roles", sub.equals("sin-rol") ? List.of() : List.of(sub.equals("docente") ? "docente" : "estudiante")))
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

    private static final String ENTRADA = """
            {"name":"Referencia","description":"Compartido","processes":[
            {"pid":2,"arrivalTime":0,"burstTime":5,"priority":-1},
            {"pid":1,"arrivalTime":3,"burstTime":2,"priority":0}]}
            """;

    @Test void cicloCompartidoYRoles() throws Exception {
        String docente = token("docente"), estudiante = token("estudiante");
        var creado = solicitar(HttpMethod.POST, "/conjuntos", docente, ENTRADA);
        assertEquals(201, creado.getStatusCode().value());
        String id = creado.getBody().path("conjuntoId").asText();
        String ruta = "/conjuntos/" + id;
        assertEquals(4, creado.getBody().size());
        assertEquals("docente", jdbc.sql("SELECT creador_sub FROM conjuntos WHERE conjunto_id=?").param(id).query(String.class).single());
        assertEquals(2, jdbc.sql("SELECT count(*) FROM procesos WHERE conjunto_id=?").param(id).query(Integer.class).single());
        assertEquals(creado.getBody(), solicitar(HttpMethod.GET, ruta, estudiante, null).getBody());
        assertTrue(solicitar(HttpMethod.GET, "/conjuntos", estudiante, null).getBody().path("items").isArray());
        assertEquals(403, solicitar(HttpMethod.POST, "/conjuntos", estudiante, ENTRADA).getStatusCode().value());
        var denegado = solicitar(HttpMethod.DELETE, ruta, estudiante, null);
        assertEquals(403, denegado.getStatusCode().value());
        assertEquals("FORBIDDEN", denegado.getBody().path("code").asText());
        assertEquals(204, solicitar(HttpMethod.DELETE, ruta, docente, null).getStatusCode().value());
        assertEquals(0, jdbc.sql("SELECT count(*) FROM procesos WHERE conjunto_id=?").param(id).query(Integer.class).single());
        assertEquals(404, solicitar(HttpMethod.GET, ruta, estudiante, null).getStatusCode().value());
        assertEquals(404, solicitar(HttpMethod.DELETE, ruta, docente, null).getStatusCode().value());
    }

    @Test void entradasInvalidasNoPersisten() throws Exception {
        int antes = jdbc.sql("SELECT count(*) FROM conjuntos").query(Integer.class).single();
        for (String cuerpo : List.of("{}", "null", "{", ENTRADA.replace("Referencia", " "),
                ENTRADA.replace("Referencia", "a".repeat(201)), ENTRADA.replace("Compartido", "a".repeat(2001)),
                ENTRADA.replace("\"pid\":1", "\"pid\":2"), ENTRADA.replace("\"pid\":2", "\"pid\":-1"),
                ENTRADA.replace("\"arrivalTime\":0", "\"arrivalTime\":-1"),
                ENTRADA.replace("\"burstTime\":5", "\"burstTime\":0"),
                ENTRADA.replace("\"burstTime\":5", "\"burstTime\":1.5"),
                "{\"name\":\"X\",\"processes\":[]}", "{\"name\":\"X\",\"processes\":[null]}")) {
            assertEquals(400, solicitar(HttpMethod.POST, "/conjuntos", token("docente"), cuerpo).getStatusCode().value(), cuerpo);
        }
        assertEquals(antes, jdbc.sql("SELECT count(*) FROM conjuntos").query(Integer.class).single());
        assertEquals(404, solicitar(HttpMethod.GET, "/conjuntos/no-uuid", token("estudiante"), null).getStatusCode().value());
    }

    @Autowired ConjuntoServicio servicio;
    @Test void revierteConjuntoSiFallaInsercionDeProceso() {
        int antes = jdbc.sql("SELECT count(*) FROM conjuntos").query(Integer.class).single();
        assertThrows(org.springframework.dao.DataIntegrityViolationException.class, () -> servicio.crear("docente",
                new ApiModelos.Solicitud("Rollback", null, List.of(new ApiModelos.Proceso(1, 0L, 1L, 0),
                        new ApiModelos.Proceso(2, 0L, 0L, 0)))));
        assertEquals(antes, jdbc.sql("SELECT count(*) FROM conjuntos").query(Integer.class).single());
    }

    @Test void jwtInvalido() throws Exception {
        for (String jwt : Arrays.asList(null, "invalido", token(""),
                token("x", "otro", "proceso-service", Instant.now().plusSeconds(300)),
                token("x", ISSUER, "planificador-service", Instant.now().plusSeconds(300)),
                token("x", ISSUER, "proceso-service", Instant.now().minusSeconds(300)))) {
            var respuesta = solicitar(HttpMethod.GET, "/conjuntos", jwt, null);
            assertEquals(401, respuesta.getStatusCode().value());
            assertEquals("UNAUTHORIZED", respuesta.getBody().path("code").asText());
        }
    }

    @Test void autenticadoSinRolesPuedeConsultar() throws Exception {
        assertEquals(200, solicitar(HttpMethod.GET, "/conjuntos", token("sin-rol"), null).getStatusCode().value());
        assertEquals(403, solicitar(HttpMethod.POST, "/conjuntos", token("sin-rol"), ENTRADA).getStatusCode().value());
    }

    @Test void erroresDeBaseRespetanContrato() throws Exception {
        // Fallos controlados de infraestructura en H2; se restaura el esquema en finally.
        jdbc.sql("ALTER TABLE conjuntos ADD CONSTRAINT ensayo_conflicto CHECK (name <> 'Conflicto')").update();
        try {
            var respuesta = solicitar(HttpMethod.POST, "/conjuntos", token("docente"), ENTRADA.replace("Referencia", "Conflicto"));
            assertEquals(409, respuesta.getStatusCode().value());
            assertEquals("CONFLICT", respuesta.getBody().path("code").asText());
        } finally {
            jdbc.sql("ALTER TABLE conjuntos DROP CONSTRAINT ensayo_conflicto").update();
        }
        jdbc.sql("ALTER TABLE conjuntos RENAME TO conjuntos_temporal").update();
        try {
            var respuesta = solicitar(HttpMethod.GET, "/conjuntos", token("estudiante"), null);
            assertEquals(500, respuesta.getStatusCode().value());
            assertEquals("SERVER_ERROR", respuesta.getBody().path("code").asText());
            assertEquals(2, respuesta.getBody().size());
        } finally {
            jdbc.sql("ALTER TABLE conjuntos_temporal RENAME TO conjuntos").update();
        }
    }
}


