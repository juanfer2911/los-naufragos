package com.losnaufragos.planificador;

import jakarta.validation.Valid;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.jwt.Jwt;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.server.ResponseStatusException;
import static org.springframework.http.HttpStatus.NOT_FOUND;
import static com.losnaufragos.planificador.ApiModelos.*;

@RestController
public class PlanificacionControlador {
    private final PlanificacionServicio servicio;
    private final PlanificacionRepositorio repositorio;

    public PlanificacionControlador(PlanificacionServicio servicio, PlanificacionRepositorio repositorio) {
        this.servicio = servicio;
        this.repositorio = repositorio;
    }

    @GetMapping("/algoritmos")
    public Catalogo catalogo() { return servicio.catalogo(); }

    @PostMapping("/planificaciones")
    public ResponseEntity<Planificacion> crear(@AuthenticationPrincipal Jwt jwt, @Valid @RequestBody Solicitud solicitud) {
        return ResponseEntity.status(201).body(servicio.ejecutar(jwt.getSubject(), solicitud));
    }

    @GetMapping("/historial")
    public Historial historial(@AuthenticationPrincipal Jwt jwt) {
        return new Historial(repositorio.listar(jwt.getSubject()));
    }

    @GetMapping("/historial/{planificacionId}")
    public Planificacion obtener(@AuthenticationPrincipal Jwt jwt, @PathVariable String planificacionId) {
        // La misma respuesta oculta si una corrida pertenece a otra persona.
        return repositorio.buscar(jwt.getSubject(), planificacionId)
                .orElseThrow(() -> new ResponseStatusException(NOT_FOUND, "Planificacion no encontrada."));
    }
}
