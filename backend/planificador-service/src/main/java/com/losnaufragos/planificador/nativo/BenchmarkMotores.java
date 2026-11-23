package com.losnaufragos.planificador.nativo;

import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.SplittableRandom;

import static com.losnaufragos.planificador.nativo.PlanificadorNativo.*;

/** Ejecuta protocolos académicos aislados del backend, su historial y las bases de datos. */
public final class BenchmarkMotores {
    private static final long SEMILLA_BASE = 20261116L;
    private static final boolean PERFIL_CONTENEDOR = Boolean.parseBoolean(
            System.getenv("BENCHMARK_CONTAINER_LIMITED"));
    private static final String[] ALGORITMOS = {"fcfs", "sjf", "srtf", "rr"};
    private static final int QUANTUM = 10;
    private final PlanificadorNativo nativo;
    private final PlanificadorJava referencia = new PlanificadorJava();
    private final Path salida;
    private final List<Medicion> mediciones = new ArrayList<>();

    private BenchmarkMotores(Path biblioteca, Path salida) {
        this.nativo = new PlanificadorNativo(biblioteca);
        this.salida = salida;
    }

    public static void main(String[] args) throws Exception {
        String protocolo = args.length == 3 ? args[0] : System.getenv("BENCHMARK_PROTOCOL");
        String biblioteca = args.length == 3 ? args[1] : System.getenv("PLANIFICADOR_BIBLIOTECA");
        String archivo = args.length == 3 ? args[2] : System.getenv("BENCHMARK_CSV");
        if (protocolo == null || !(protocolo.equals("web2") || protocolo.equals("arquitectura"))
                || biblioteca == null || archivo == null) {
            throw new IllegalArgumentException("Configure BENCHMARK_PROTOCOL, PLANIFICADOR_BIBLIOTECA y BENCHMARK_CSV.");
        }
        var benchmark = new BenchmarkMotores(Path.of(biblioteca), Path.of(archivo));
        if (protocolo.equals("web2")) benchmark.web2(); else benchmark.arquitectura();
        benchmark.escribirResultados(protocolo);
    }

    private void web2() {
        for (int cantidad : new int[]{50, 500, 5_000, 50_000}) {
            List<Proceso> entrada = generar(cantidad, SEMILLA_BASE + cantidad);
            for (String algoritmo : ALGORITMOS) {
                for (int repeticion = 1; repeticion <= 30; repeticion++) {
                    medir("web2", algoritmo, cantidad, repeticion, false, entrada);
                    if (PERFIL_CONTENEDOR && cantidad >= 5_000) {
                        // Libera los buffers nativos de JNA entre lotes sin incluir GC en el tiempo medido.
                        System.gc();
                    }
                    if (cantidad == 50_000 && repeticion % 10 == 0) {
                        System.out.printf("Web 2: 50.000 procesos, %s, repetición %d/30.%n", algoritmo, repeticion);
                    }
                }
                System.out.printf("Web 2: %d procesos, %s, 30 repeticiones completadas.%n", cantidad, algoritmo);
            }
        }
    }

    private void arquitectura() {
        List<Proceso> entrada = generar(10_000, SEMILLA_BASE + 10_000L);
        for (int repeticion = 1; repeticion <= 5; repeticion++) {
            medir("arquitectura", "srtf", 10_000, repeticion, repeticion <= 3, entrada);
            System.out.printf("Arquitectura: repeticion %d/5 completada (%s).%n", repeticion,
                    repeticion <= 3 ? "calentamiento" : "medicion");
        }
    }

    private void medir(String protocolo, String algoritmo, int cantidad, int repeticion,
                       boolean calentamiento, List<Proceso> entrada) {
        long quantum = algoritmo.equals("rr") ? QUANTUM : 0;
        Resultado resultadoC;
        Resultado resultadoJava;
        long nanosC, nanosJava;
        if ((repeticion & 1) == 1) {
            var tomada = ejecutarCronometrado(nativo, algoritmo, entrada, quantum);
            resultadoC = tomada.resultado(); nanosC = tomada.nanos();
            tomada = ejecutarCronometrado(referencia, algoritmo, entrada, quantum);
            resultadoJava = tomada.resultado(); nanosJava = tomada.nanos();
        } else {
            var tomada = ejecutarCronometrado(referencia, algoritmo, entrada, quantum);
            resultadoJava = tomada.resultado(); nanosJava = tomada.nanos();
            tomada = ejecutarCronometrado(nativo, algoritmo, entrada, quantum);
            resultadoC = tomada.resultado(); nanosC = tomada.nanos();
        }
        if (!resultadoC.equals(resultadoJava)) {
            throw new IllegalStateException("C y Java difieren: " + protocolo + ", " + algoritmo
                    + ", procesos=" + cantidad + ", repeticion=" + repeticion + ". "
                    + describirDiferencia(resultadoC, resultadoJava));
        }
        mediciones.add(new Medicion(protocolo, algoritmo, cantidad, repeticion, "c_jna", nanosC, calentamiento));
        mediciones.add(new Medicion(protocolo, algoritmo, cantidad, repeticion, "java", nanosJava, calentamiento));
    }

    private static String describirDiferencia(Resultado c, Resultado referencia) {
        if (!c.segmentos().equals(referencia.segmentos())) {
            int limite = Math.min(c.segmentos().size(), referencia.segmentos().size());
            for (int i = 0; i < limite; i++) if (!c.segmentos().get(i).equals(referencia.segmentos().get(i))) {
                return "segmento " + i + ": C=" + c.segmentos().get(i) + ", Java=" + referencia.segmentos().get(i);
            }
            return "cantidad de segmentos: C=" + c.segmentos().size() + ", Java=" + referencia.segmentos().size();
        }
        int limite = Math.min(c.metricas().size(), referencia.metricas().size());
        for (int i = 0; i < limite; i++) if (!c.metricas().get(i).equals(referencia.metricas().get(i))) {
            return "metrica " + i + ": C=" + c.metricas().get(i) + ", Java=" + referencia.metricas().get(i);
        }
        return "cantidad de metricas: C=" + c.metricas().size() + ", Java=" + referencia.metricas().size();
    }

    private static Tomada ejecutarCronometrado(MotorPlanificacion motor, String algoritmo,
                                                List<Proceso> entrada, long quantum) {
        long inicio = System.nanoTime();
        Resultado resultado = motor.ejecutar(algoritmo, entrada, quantum);
        return new Tomada(resultado, System.nanoTime() - inicio);
    }

    private static List<Proceso> generar(int cantidad, long semilla) {
        var random = new SplittableRandom(semilla);
        var procesos = new ArrayList<Proceso>(cantidad);
        long llegadaMaxima = Math.multiplyExact((long) cantidad, 2L);
        for (int i = 0; i < cantidad; i++) {
            procesos.add(new Proceso(i + 1, random.nextLong(llegadaMaxima + 1),
                    random.nextLong(1, 101), random.nextInt(1, 6)));
        }
        return List.copyOf(procesos);
    }

    private void escribirResultados(String protocolo) throws IOException {
        if (salida.getParent() != null) Files.createDirectories(salida.getParent());
        try (BufferedWriter writer = Files.newBufferedWriter(salida)) {
            writer.write("protocolo,semilla,algoritmo,procesos,repeticion,motor,tiempo_ns,calentamiento,coinciden\n");
            for (Medicion m : mediciones) {
                writer.write(String.join(",", m.protocolo(), Long.toString(SEMILLA_BASE + m.procesos()),
                        m.algoritmo(), Integer.toString(m.procesos()), Integer.toString(m.repeticion()),
                        m.motor(), Long.toString(m.nanos()), Boolean.toString(m.calentamiento()), "true"));
                writer.newLine();
            }
        }
        Path resumen = salida.resolveSibling(salida.getFileName().toString().replaceFirst("\\.csv$", "") + "-medianas.csv");
        try (BufferedWriter writer = Files.newBufferedWriter(resumen)) {
            writer.write("protocolo,algoritmo,procesos,motor,repeticiones,mediana_ns\n");
            var grupos = mediciones.stream().collect(java.util.stream.Collectors.groupingBy(
                    m -> m.protocolo() + "," + m.algoritmo() + "," + m.procesos() + "," + m.motor(),
                    java.util.TreeMap::new, java.util.stream.Collectors.toList()));
            for (var grupo : grupos.entrySet()) {
                List<Long> tiempos = grupo.getValue().stream().filter(m -> !m.calentamiento())
                        .map(Medicion::nanos).sorted(Comparator.naturalOrder()).toList();
                if (!tiempos.isEmpty()) {
                    double mediana = tiempos.size() % 2 == 1 ? tiempos.get(tiempos.size() / 2)
                            : (tiempos.get(tiempos.size() / 2 - 1) / 2.0 + tiempos.get(tiempos.size() / 2) / 2.0);
                    writer.write(grupo.getKey() + "," + tiempos.size() + "," + mediana);
                    writer.newLine();
                }
            }
        }
        System.out.println("Resultados crudos: " + salida.toAbsolutePath());
        System.out.println("Medianas: " + resumen.toAbsolutePath());
    }

    private record Tomada(Resultado resultado, long nanos) { }
    private record Medicion(String protocolo, String algoritmo, int procesos, int repeticion,
                            String motor, long nanos, boolean calentamiento) { }
}
