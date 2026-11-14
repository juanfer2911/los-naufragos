package com.losnaufragos.planificador;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import jakarta.validation.Valid;
import jakarta.validation.constraints.*;
import java.time.Instant;
import java.util.List;

public final class ApiModelos {
    private ApiModelos() { }

    public record Proceso(@NotNull @Min(0) Integer pid,
                          @NotNull @Min(0) Long arrivalTime,
                          @NotNull @Min(1) Long burstTime,
                          @NotNull Integer priority) { }
    public record Solicitud(@NotBlank String algorithmId,
                            @NotEmpty List<@NotNull @Valid Proceso> processes,
                            @Min(1) Long quantum, String sourceSetId) { }
    public record Algoritmo(String id) { }
    public record Catalogo(List<Algoritmo> algorithms) { }
    @JsonInclude(JsonInclude.Include.NON_NULL)
    public record Segmento(String kind, Integer pid, long start, long end) { }
    public record Metrica(int pid, long arrivalTime, long burstTime, long completionTime,
                          long turnaroundTime, long waitingTime, long responseTime) { }
    public record Promedios(double waitingTime, double turnaroundTime, double responseTime) { }
    @JsonInclude(JsonInclude.Include.NON_NULL)
    public record Planificacion(String algorithmId, List<Segmento> segments,
                                List<Metrica> processMetrics, Promedios averages,
                                String planificationId, Instant createdAt,
                                List<Proceso> processes, String sourceSetId,
                                @JsonProperty(access = JsonProperty.Access.WRITE_ONLY) Long quantum) { }
    public record Historial(List<Planificacion> items) { }
    public record Error(String code, String message) { }
}
