package com.losnaufragos.planificador;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.server.ResponseStatusException;

@RestControllerAdvice
public class ErroresApi {
    private static final Logger LOG = LoggerFactory.getLogger(ErroresApi.class);

    @ExceptionHandler({IllegalArgumentException.class, MethodArgumentNotValidException.class,
            HttpMessageNotReadableException.class})
    ResponseEntity<ApiModelos.Error> entrada(Exception error) {
        String mensaje = error instanceof IllegalArgumentException ? error.getMessage() : "Revise los campos y tipos de la solicitud.";
        return ResponseEntity.badRequest().body(new ApiModelos.Error("BAD_REQUEST", mensaje));
    }

    @ExceptionHandler(ResponseStatusException.class)
    ResponseEntity<ApiModelos.Error> estado(ResponseStatusException error) {
        return ResponseEntity.status(error.getStatusCode()).body(new ApiModelos.Error("NOT_FOUND", error.getReason()));
    }

    @ExceptionHandler({Exception.class, UnsatisfiedLinkError.class})
    ResponseEntity<ApiModelos.Error> interno(Throwable error) {
        LOG.error("No se pudo completar la operacion", error);
        return ResponseEntity.internalServerError().body(new ApiModelos.Error("SERVER_ERROR", "No se pudo completar la operacion."));
    }
}
