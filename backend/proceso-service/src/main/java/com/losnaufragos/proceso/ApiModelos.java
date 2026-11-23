package com.losnaufragos.proceso;

import com.fasterxml.jackson.annotation.JsonInclude;
import jakarta.validation.Valid;
import jakarta.validation.constraints.*;
import java.util.List;

public final class ApiModelos {
    private ApiModelos() { }
    public record Proceso(@NotNull(message = "es obligatorio") @Min(value = 0, message = "debe ser mayor o igual que 0") Integer pid,
                          @NotNull(message = "es obligatorio") @Min(value = 0, message = "debe ser mayor o igual que 0") Long arrivalTime,
                          @NotNull(message = "es obligatorio") @Min(value = 1, message = "debe ser mayor o igual que 1") Long burstTime,
                          @NotNull(message = "es obligatorio") Integer priority) { }
    public record Solicitud(@NotBlank(message = "es obligatorio y no puede estar vacío") @Size(max=200, message = "no puede superar 200 caracteres") String name,
                            @Size(max=2000, message = "no puede superar 2000 caracteres") String description,
                            @NotEmpty(message = "debe contener al menos un proceso") List<@NotNull(message = "no puede ser nulo") @Valid Proceso> processes,
                            @Min(value = 1, message = "debe ser mayor o igual que 1") Long quantum) { }
    @JsonInclude(JsonInclude.Include.NON_NULL)
    public record Conjunto(String conjuntoId, String name, String description, List<Proceso> processes, Long quantum) { }
    public record Lista(List<Conjunto> items) { }
    public record Error(String code, String message) { }
}
