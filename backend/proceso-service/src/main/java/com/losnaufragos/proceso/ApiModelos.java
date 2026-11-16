package com.losnaufragos.proceso;

import com.fasterxml.jackson.annotation.JsonInclude;
import jakarta.validation.Valid;
import jakarta.validation.constraints.*;
import java.util.List;

public final class ApiModelos {
    private ApiModelos() { }
    public record Proceso(@NotNull @Min(0) Integer pid, @NotNull @Min(0) Long arrivalTime,
                          @NotNull @Min(1) Long burstTime, @NotNull Integer priority) { }
    public record Solicitud(@NotBlank @Size(max=200) String name, @Size(max=2000) String description,
                            @NotEmpty List<@NotNull @Valid Proceso> processes, @Min(1) Long quantum) { }
    @JsonInclude(JsonInclude.Include.NON_NULL)
    public record Conjunto(String conjuntoId, String name, String description, List<Proceso> processes, Long quantum) { }
    public record Lista(List<Conjunto> items) { }
    public record Error(String code, String message) { }
}
