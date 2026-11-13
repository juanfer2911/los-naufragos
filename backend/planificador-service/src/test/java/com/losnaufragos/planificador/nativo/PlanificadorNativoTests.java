package com.losnaufragos.planificador.nativo;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.condition.EnabledIfSystemProperty;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

import java.util.List;

import static com.losnaufragos.planificador.nativo.PlanificadorNativo.*;
import static org.junit.jupiter.api.Assertions.*;

// Activacion explicita: estas pruebas necesitan la biblioteca Linux compilada.
@EnabledIfSystemProperty(named = "planificador.biblioteca", matches = ".+")
class PlanificadorNativoTests {
    private static PlanificadorNativo motor;
    private static final List<Proceso> PROCESOS = List.of(
            new Proceso(1, 1, 5, 0), new Proceso(2, 2, 1, 0), new Proceso(3, 2, 2, 0));

    @BeforeAll
    static void cargar() {
        motor = PlanificadorNativo.desdeConfiguracion();
    }

    @ParameterizedTest
    @ValueSource(strings = {"fcfs", "sjf", "srtf", "rr"})
    void ejecutaAlgoritmosConResultadosEsperados(String id) {
        Resultado resultado = motor.ejecutar(id, PROCESOS, 2);
        List<Segmento> segmentos = switch (id) {
            case "srtf" -> List.of(new Segmento(0, 0, 1, true),
                    new Segmento(1, 1, 2, false), new Segmento(2, 2, 3, false),
                    new Segmento(3, 3, 5, false), new Segmento(1, 5, 9, false));
            case "rr" -> List.of(new Segmento(0, 0, 1, true),
                    new Segmento(1, 1, 3, false), new Segmento(2, 3, 4, false),
                    new Segmento(3, 4, 6, false), new Segmento(1, 6, 9, false));
            default -> List.of(new Segmento(0, 0, 1, true),
                    new Segmento(1, 1, 6, false), new Segmento(2, 6, 7, false),
                    new Segmento(3, 7, 9, false));
        };
        List<Metrica> metricas = switch (id) {
            case "srtf" -> List.of(new Metrica(1, 3, 8, 0),
                    new Metrica(2, 0, 1, 0), new Metrica(3, 1, 3, 1));
            case "rr" -> List.of(new Metrica(1, 3, 8, 0),
                    new Metrica(2, 1, 2, 1), new Metrica(3, 2, 4, 2));
            default -> List.of(new Metrica(1, 0, 5, 0),
                    new Metrica(2, 4, 5, 4), new Metrica(3, 5, 7, 5));
        };
        assertEquals(new Resultado(segmentos, metricas), resultado);
    }

    @Test
    void sjfSeleccionaLaRafagaMenor() {
        var entrada = List.of(new Proceso(1, 1, 5, 0), new Proceso(2, 1, 1, 0));
        assertEquals(List.of(new Segmento(0, 0, 1, true), new Segmento(2, 1, 2, false),
                new Segmento(1, 2, 7, false)), motor.ejecutar("sjf", entrada, 0).segmentos());
    }

    @Test
    void quantumSeTransmiteAC() {
        var corto = motor.ejecutar("rr", PROCESOS, 1);
        var largo = motor.ejecutar("rr", PROCESOS, 20);
        assertEquals(new Segmento(1, 1, 2, false), corto.segmentos().get(1));
        assertEquals(motor.ejecutar("fcfs", PROCESOS, 0), largo);
        assertNotEquals(corto, largo);
    }

    @Test
    void catalogoProcedeDeC() {
        assertEquals(List.of(new Algoritmo("fcfs", "FCFS/FIFO", false),
                new Algoritmo("sjf", "SJF", false), new Algoritmo("srtf", "SRTF", false),
                new Algoritmo("rr", "Round Robin", true)), motor.listarAlgoritmos());
        for (Algoritmo algoritmo : motor.listarAlgoritmos()) {
            assertFalse(motor.ejecutar(algoritmo.id(), PROCESOS,
                    algoritmo.requiereQuantum() ? 2 : 0).metricas().isEmpty());
        }
    }

    @Test
    void erroresNativosSonComprensibles() {
        var error = assertThrows(IllegalArgumentException.class,
                () -> motor.ejecutar("rr", PROCESOS, 0));
        assertTrue(error.getMessage().contains("Codigo: 1"));
        assertThrows(IllegalArgumentException.class, () -> motor.ejecutar("desconocido", PROCESOS, 0));
    }

    @Test
    void estructurasConservanElAnchoDeLosTiempos() {
        long llegada = 3_000_000_000L;
        var resultado = motor.ejecutar("fcfs", List.of(new Proceso(7, llegada, 2, -3)), 0);
        assertEquals(new Segmento(7, llegada, llegada + 2, false), resultado.segmentos().get(1));
        assertEquals(List.of(new Metrica(7, 0, 2, 0)), resultado.metricas());
    }
}
