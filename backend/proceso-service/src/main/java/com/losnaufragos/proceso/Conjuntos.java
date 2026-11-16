package com.losnaufragos.proceso;

import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.jwt.Jwt;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.server.ResponseStatusException;
import java.util.*;
import static com.losnaufragos.proceso.ApiModelos.*;

@RestController
@RequestMapping("/conjuntos")
public class Conjuntos {
    private final ConjuntoServicio servicio;
    public Conjuntos(ConjuntoServicio servicio) { this.servicio = servicio; }
    @GetMapping public Lista listar() { return servicio.listar(); }
    @GetMapping("/{id}") public Conjunto obtener(@PathVariable String id) { return servicio.obtener(id); }
    @PostMapping @ResponseStatus(HttpStatus.CREATED)
    public Conjunto crear(@AuthenticationPrincipal Jwt jwt, @Valid @RequestBody Solicitud solicitud) {
        return servicio.crear(jwt.getSubject(), solicitud);
    }
    @DeleteMapping("/{id}") @ResponseStatus(HttpStatus.NO_CONTENT)
    public void eliminar(@PathVariable String id) { servicio.eliminar(id); }
}

@Service
class ConjuntoServicio {
    private final JdbcClient jdbc;
    ConjuntoServicio(JdbcClient jdbc) { this.jdbc = jdbc; }

    @Transactional(readOnly=true, isolation=org.springframework.transaction.annotation.Isolation.REPEATABLE_READ)
    public Lista listar() {
        var ids = jdbc.sql("SELECT conjunto_id FROM conjuntos ORDER BY conjunto_id").query(String.class).list();
        return new Lista(ids.stream().map(this::obtener).toList());
    }

    @Transactional(readOnly=true, isolation=org.springframework.transaction.annotation.Isolation.REPEATABLE_READ)
    public Conjunto obtener(String id) {
        validarId(id);
        return jdbc.sql("SELECT conjunto_id, name, description, quantum FROM conjuntos WHERE conjunto_id = ?").param(id)
                .query((rs, row) -> new Conjunto(rs.getString(1), rs.getString(2), rs.getString(3), procesos(id), rs.getObject(4, Long.class)))
                .optional().orElseThrow(ConjuntoServicio::noEncontrado);
    }

    private List<Proceso> procesos(String id) {
        return jdbc.sql("SELECT pid, arrival_time, burst_time, priority FROM procesos WHERE conjunto_id = ? ORDER BY posicion")
                .param(id).query((rs, row) -> new Proceso(rs.getInt(1), rs.getLong(2), rs.getLong(3), rs.getInt(4))).list();
    }

    @Transactional
    public Conjunto crear(String sub, Solicitud solicitud) {
        var ids = new HashSet<Integer>();
        for (var proceso : solicitud.processes()) {
            if (!ids.add(proceso.pid())) throw new IllegalArgumentException("Los pid deben ser unicos dentro de processes.");
        }
        String id = UUID.randomUUID().toString();
        jdbc.sql("INSERT INTO conjuntos(conjunto_id, name, description, creador_sub, quantum) VALUES (?, ?, ?, ?, ?)")
                .params(Arrays.asList(id, solicitud.name(), solicitud.description(), sub, solicitud.quantum())).update();
        int posicion = 0;
        for (var p : solicitud.processes()) {
            jdbc.sql("INSERT INTO procesos(conjunto_id, posicion, pid, arrival_time, burst_time, priority) VALUES (?, ?, ?, ?, ?, ?)")
                    .params(id, posicion++, p.pid(), p.arrivalTime(), p.burstTime(), p.priority()).update();
        }
        return new Conjunto(id, solicitud.name(), solicitud.description(), List.copyOf(solicitud.processes()), solicitud.quantum());
    }

    @Transactional
    public void eliminar(String id) {
        validarId(id);
        if (jdbc.sql("DELETE FROM conjuntos WHERE conjunto_id = ?").param(id).update() == 0) throw noEncontrado();
    }
    private static void validarId(String id) {
        // OpenAPI no define 400 para estas rutas: un identificador no canonico no encuentra recurso.
        try { if (!UUID.fromString(id).toString().equals(id)) throw noEncontrado(); }
        catch (IllegalArgumentException e) { throw noEncontrado(); }
    }
    private static ResponseStatusException noEncontrado() {
        return new ResponseStatusException(HttpStatus.NOT_FOUND, "Conjunto no encontrado.");
    }
}
