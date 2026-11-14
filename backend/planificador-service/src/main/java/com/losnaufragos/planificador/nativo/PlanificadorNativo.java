package com.losnaufragos.planificador.nativo;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Platform;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public final class PlanificadorNativo {
    public record Proceso(int id, long llegada, long rafaga, int prioridad) { }
    public record Segmento(int procesoId, long inicio, long fin, boolean ocioso) { }
    public record Metrica(int procesoId, long espera, long retorno, long respuesta) { }
    public record Algoritmo(String id, String nombre, boolean requiereQuantum) { }
    public record Resultado(List<Segmento> segmentos, List<Metrica> metricas) { }

    private final PlanificadorBiblioteca biblioteca;

    public PlanificadorNativo(Path archivo) {
        if (!Platform.isLinux() || Native.POINTER_SIZE != 8) {
            throw new IllegalStateException("La biblioteca requiere una JVM Linux de 64 bits (Ubuntu/WSL).");
        }
        Objects.requireNonNull(archivo, "Indique la ruta de libplanificador.so");
        if (!archivo.isAbsolute() || !Files.isRegularFile(archivo)) {
            throw new IllegalArgumentException("Se requiere una ruta absoluta a libplanificador.so: " + archivo);
        }
        biblioteca = Native.load(archivo.toString(), PlanificadorBiblioteca.class,
                Map.of(Library.OPTION_STRING_ENCODING, "UTF-8"));
        if (new PlanificadorBiblioteca.Proceso().size() != 32
                || new PlanificadorBiblioteca.Segmento().size() != 32
                || new PlanificadorBiblioteca.Metrica().size() != 32
                || new PlanificadorBiblioteca.Algoritmo().size() != 84) {
            throw new IllegalStateException("La disposicion de estructuras JNA no coincide con la ABI esperada.");
        }
    }

    public static PlanificadorNativo desdeConfiguracion() {
        String ruta = System.getProperty("planificador.biblioteca",
                System.getenv("PLANIFICADOR_BIBLIOTECA"));
        if (ruta == null || ruta.isBlank()) {
            throw new IllegalStateException("Configure -Dplanificador.biblioteca o PLANIFICADOR_BIBLIOTECA con la ruta Linux absoluta.");
        }
        return new PlanificadorNativo(Path.of(ruta));
    }

    public List<Algoritmo> listarAlgoritmos() {
        IntByReference cantidad = new IntByReference();
        Pointer memoria = biblioteca.listar_algoritmos(cantidad);
        if (memoria == null || cantidad.getValue() <= 0) {
            throw new IllegalStateException("El catalogo nativo no esta disponible.");
        }
        List<Algoritmo> resultado = new ArrayList<>();
        var primero = new PlanificadorBiblioteca.Algoritmo(memoria);
        for (var algoritmo : (PlanificadorBiblioteca.Algoritmo[]) primero.toArray(cantidad.getValue())) {
            algoritmo.read();
            resultado.add(new Algoritmo(Native.toString(algoritmo.id, "UTF-8"),
                    Native.toString(algoritmo.nombre, "UTF-8"), algoritmo.requiereQuantum != 0));
        }
        return List.copyOf(resultado);
    }

    public Resultado ejecutar(String algoritmoId, List<Proceso> entrada, long quantum) {
        if (algoritmoId == null || algoritmoId.isBlank() || algoritmoId.indexOf('\0') >= 0
                || entrada == null || entrada.isEmpty()) {
            throw new IllegalArgumentException("Indique un algoritmo y al menos un proceso.");
        }
        // Copia contigua: C recibe un array, no una lista de punteros.
        var procesos = (PlanificadorBiblioteca.Proceso[]) new PlanificadorBiblioteca.Proceso().toArray(entrada.size());
        for (int i = 0; i < entrada.size(); i++) {
            Proceso origen = Objects.requireNonNull(entrada.get(i), "Proceso nulo");
            procesos[i].id = origen.id();
            procesos[i].llegada = origen.llegada();
            procesos[i].rafaga = origen.rafaga();
            procesos[i].prioridad = origen.prioridad();
            procesos[i].write();
        }
        IntByReference ns = new IntByReference();
        IntByReference nm = new IntByReference();
        int estado = biblioteca.planificador_ejecutar_v1(algoritmoId, procesos[0].getPointer(),
                procesos.length, quantum, null, 0, ns, null, 0, nm);
        comprobarEstado(estado, 3);
        if (ns.getValue() <= 0 || nm.getValue() != procesos.length) {
            throw new IllegalStateException("El sondeo nativo devolvio cantidades inconsistentes.");
        }
        var segmentos = (PlanificadorBiblioteca.Segmento[]) new PlanificadorBiblioteca.Segmento().toArray(ns.getValue());
        var metricas = (PlanificadorBiblioteca.Metrica[]) new PlanificadorBiblioteca.Metrica().toArray(nm.getValue());
        estado = biblioteca.planificador_ejecutar_v1(algoritmoId, procesos[0].getPointer(),
                procesos.length, quantum, segmentos[0].getPointer(), segmentos.length, ns,
                metricas[0].getPointer(), metricas.length, nm);
        comprobarEstado(estado, 0);
        List<Segmento> salidaSegmentos = new ArrayList<>();
        for (var segmento : segmentos) {
            segmento.read();
            salidaSegmentos.add(new Segmento(segmento.procesoId, segmento.inicio, segmento.fin, segmento.esOcioso != 0));
        }
        List<Metrica> salidaMetricas = new ArrayList<>();
        for (var metrica : metricas) {
            metrica.read();
            salidaMetricas.add(new Metrica(metrica.procesoId, metrica.espera, metrica.retorno, metrica.respuesta));
        }
        return new Resultado(List.copyOf(salidaSegmentos), List.copyOf(salidaMetricas));
    }

    private static void comprobarEstado(int estado, int esperado) {
        if (estado == esperado) return;
        String mensaje = switch (estado) {
            case 1 -> "Argumento nativo invalido: revise algoritmo, procesos y quantum.";
            case 2 -> "Memoria nativa insuficiente.";
            case 3 -> "Capacidad de resultados insuficiente.";
            case 4 -> "Desbordamiento de tiempos nativos.";
            case 5 -> "Cantidad de segmentos no representable en int32_t.";
            default -> "Estado nativo inesperado.";
        };
        if (estado == 1) throw new IllegalArgumentException(mensaje + " Codigo: " + estado);
        throw new IllegalStateException(mensaje + " Codigo: " + estado);
    }
}
