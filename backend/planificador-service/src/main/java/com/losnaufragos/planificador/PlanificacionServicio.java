package com.losnaufragos.planificador;

import com.losnaufragos.planificador.nativo.PlanificadorNativo;
import org.springframework.beans.factory.ObjectProvider;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.*;
import org.springframework.stereotype.Service;
import java.nio.file.Path;
import java.time.Instant;
import java.util.*;
import static com.losnaufragos.planificador.ApiModelos.*;

@Service
public class PlanificacionServicio {
    private final ObjectProvider<PlanificadorNativo> motor;
    private final PlanificacionRepositorio repositorio;

    public PlanificacionServicio(ObjectProvider<PlanificadorNativo> motor, PlanificacionRepositorio repositorio) {
        this.motor = motor;
        this.repositorio = repositorio;
    }

    @Configuration
    static class ConfiguracionNativa {
        @Bean @Lazy
        PlanificadorNativo motor(@Value("${planificador.biblioteca}") String ruta) {
            if (ruta.isBlank()) throw new IllegalStateException("Configure PLANIFICADOR_BIBLIOTECA con la ruta Linux absoluta.");
            return new PlanificadorNativo(Path.of(ruta));
        }
    }

    public Catalogo catalogo() {
        return new Catalogo(motor.getObject().listarAlgoritmos().stream().map(a -> new Algoritmo(a.id())).toList());
    }

    public Planificacion ejecutar(String propietario, Solicitud solicitud) {
        var nativo = motor.getObject();
        var algoritmo = nativo.listarAlgoritmos().stream().filter(a -> a.id().equals(solicitud.algorithmId()))
                .findFirst().orElseThrow(() -> new IllegalArgumentException("algorithmId no disponible en el catalogo."));
        if (algoritmo.requiereQuantum() && solicitud.quantum() == null) {
            throw new IllegalArgumentException("quantum es obligatorio para el algoritmo seleccionado.");
        }
        Set<Integer> ids = new HashSet<>();
        for (Proceso proceso : solicitud.processes()) {
            if (!ids.add(proceso.pid())) throw new IllegalArgumentException("Los pid deben ser unicos dentro de processes.");
        }
        var procesos = solicitud.processes().stream().map(p ->
                new PlanificadorNativo.Proceso(p.pid(), p.arrivalTime(), p.burstTime(), p.priority())).toList();
        var resultado = nativo.ejecutar(solicitud.algorithmId(), procesos,
                solicitud.quantum() == null ? 0 : solicitud.quantum());
        var segmentos = resultado.segmentos().stream().map(s ->
                new Segmento(s.ocioso() ? "idle" : "execution", s.ocioso() ? null : s.procesoId(), s.inicio(), s.fin())).toList();
        List<Metrica> metricas = new ArrayList<>();
        for (int i = 0; i < procesos.size(); i++) {
            var proceso = solicitud.processes().get(i);
            var metrica = resultado.metricas().get(i);
            metricas.add(new Metrica(proceso.pid(), proceso.arrivalTime(), proceso.burstTime(),
                    Math.addExact(proceso.arrivalTime(), metrica.retorno()),
                    metrica.retorno(), metrica.espera(), metrica.respuesta()));
        }
        var promedios = new Promedios(metricas.stream().mapToDouble(Metrica::waitingTime).average().orElseThrow(),
                metricas.stream().mapToDouble(Metrica::turnaroundTime).average().orElseThrow(),
                metricas.stream().mapToDouble(Metrica::responseTime).average().orElseThrow());
        var respuesta = new Planificacion(solicitud.algorithmId(), segmentos, List.copyOf(metricas), promedios,
                UUID.randomUUID().toString(), Instant.now(), List.copyOf(solicitud.processes()), solicitud.sourceSetId(),
                algoritmo.requiereQuantum() ? solicitud.quantum() : null);
        repositorio.guardar(propietario, respuesta);
        return respuesta;
    }
}
