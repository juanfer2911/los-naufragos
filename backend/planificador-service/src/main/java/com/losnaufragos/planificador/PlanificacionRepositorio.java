package com.losnaufragos.planificador;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.stereotype.Repository;
import java.sql.Timestamp;
import java.util.List;
import java.util.Optional;
import static com.losnaufragos.planificador.ApiModelos.*;

@Repository
public class PlanificacionRepositorio {
    private final JdbcClient jdbc;
    private final ObjectMapper json;

    public PlanificacionRepositorio(JdbcClient jdbc, ObjectMapper json) {
        this.jdbc = jdbc;
        this.json = json;
    }

    public void guardar(String propietario, Planificacion resultado) {
        try {
            // El snapshot conserva quantum; la serializacion HTTP lo omite.
            ObjectNode snapshot = json.valueToTree(resultado);
            if (resultado.quantum() != null) snapshot.put("quantum", resultado.quantum());
            jdbc.sql("INSERT INTO planificaciones (id, propietario, creado_en, resultado) VALUES (:id, :sub, :fecha, :json)")
                    .param("id", resultado.planificationId()).param("sub", propietario)
                    .param("fecha", Timestamp.from(resultado.createdAt()))
                    .param("json", json.writeValueAsString(snapshot)).update();
        } catch (JsonProcessingException e) {
            throw new IllegalStateException("No se pudo serializar la planificacion.", e);
        }
    }

    public List<Planificacion> listar(String propietario) {
        return jdbc.sql("SELECT resultado FROM planificaciones WHERE propietario = :sub ORDER BY creado_en DESC, id")
                .param("sub", propietario).query((rs, fila) -> leer(rs.getString(1))).list();
    }

    public Optional<Planificacion> buscar(String propietario, String id) {
        return jdbc.sql("SELECT resultado FROM planificaciones WHERE propietario = :sub AND id = :id")
                .param("sub", propietario).param("id", id)
                .query((rs, fila) -> leer(rs.getString(1))).optional();
    }

    private Planificacion leer(String valor) {
        try {
            return json.readValue(valor, Planificacion.class);
        } catch (JsonProcessingException e) {
            throw new IllegalStateException("No se pudo leer la planificacion almacenada.", e);
        }
    }
}
