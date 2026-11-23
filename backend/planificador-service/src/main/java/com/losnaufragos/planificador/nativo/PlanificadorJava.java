package com.losnaufragos.planificador.nativo;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.PriorityQueue;
import java.util.Set;

import static com.losnaufragos.planificador.nativo.PlanificadorNativo.*;

/** Motor Java de referencia: replica las políticas del núcleo C para los cuatro algoritmos base. */
public final class PlanificadorJava implements MotorPlanificacion {
    private static final List<Algoritmo> CATALOGO = List.of(
            new Algoritmo("fcfs", "FCFS/FIFO", false),
            new Algoritmo("sjf", "SJF", false),
            new Algoritmo("srtf", "SRTF", false),
            new Algoritmo("rr", "Round Robin", true));

    @Override
    public List<Algoritmo> listarAlgoritmos() { return CATALOGO; }

    @Override
    public Resultado ejecutar(String algoritmoId, List<Proceso> entrada, long quantum) {
        validar(algoritmoId, entrada, quantum);
        var simulador = new Simulador(entrada);
        switch (algoritmoId) {
            case "fcfs" -> simulador.fcfs();
            case "sjf" -> simulador.sjf();
            case "srtf" -> simulador.srtf();
            case "rr" -> simulador.rr(quantum);
            default -> throw new IllegalArgumentException("Algoritmo no disponible en el motor Java: " + algoritmoId);
        }
        return simulador.resultado();
    }

    private static void validar(String id, List<Proceso> procesos, long quantum) {
        if (id == null || CATALOGO.stream().noneMatch(a -> a.id().equals(id))) {
            throw new IllegalArgumentException("Indique un algoritmo disponible en el motor Java.");
        }
        if (procesos == null || procesos.isEmpty()) throw new IllegalArgumentException("Indique al menos un proceso.");
        Set<Integer> ids = new HashSet<>();
        for (Proceso p : procesos) {
            if (p == null || p.id() < 0 || p.llegada() < 0 || p.rafaga() < 1 || !ids.add(p.id())) {
                throw new IllegalArgumentException("Procesos invalidos: PID unico no negativo, llegada no negativa y rafaga positiva.");
            }
        }
        if (id.equals("rr") ? quantum < 1 : quantum < 0) {
            throw new IllegalArgumentException("Round Robin requiere quantum positivo.");
        }
    }

    // Comparadores locales se construyen por simulador para evitar estado compartido entre solicitudes.
    private static final class Simulador {
        private final List<Proceso> procesos;
        private final int n;
        private final List<Integer> llegadas;
        private final long[] restante;
        private final long[] primerInicio;
        private final long[] fin;
        private final List<Segmento> segmentos = new ArrayList<>();
        private long reloj;

        Simulador(List<Proceso> entrada) {
            procesos = List.copyOf(entrada); n = procesos.size(); restante = new long[n];
            primerInicio = new long[n]; fin = new long[n];
            java.util.Arrays.fill(primerInicio, -1); java.util.Arrays.fill(fin, -1);
            for (int i = 0; i < n; i++) restante[i] = procesos.get(i).rafaga();
            llegadas = new ArrayList<>(); for (int i = 0; i < n; i++) llegadas.add(i);
            llegadas.sort(Comparator.comparingLong((Integer i) -> procesos.get(i).llegada())
                    .thenComparingInt(i -> procesos.get(i).prioridad()).thenComparingInt(Integer::intValue));
        }

        private Comparator<Integer> porLlegada() { return Comparator
                .comparingLong((Integer i) -> procesos.get(i).llegada())
                .thenComparingInt(i -> procesos.get(i).prioridad()).thenComparingInt(Integer::intValue); }
        private Comparator<Integer> porSjf() { return Comparator
                .comparingLong((Integer i) -> procesos.get(i).rafaga())
                .thenComparingInt(i -> procesos.get(i).prioridad())
                .thenComparingLong(i -> procesos.get(i).llegada()).thenComparingInt(Integer::intValue); }
        private Comparator<Integer> porSrtf() { return Comparator
                .comparingLong((Integer i) -> restante[i])
                .thenComparingInt(i -> procesos.get(i).prioridad())
                .thenComparingLong(i -> procesos.get(i).llegada()).thenComparingInt(Integer::intValue); }

        void fcfs() {
            PriorityQueue<Integer> listos = new PriorityQueue<>(porLlegada());
            int siguiente = 0;
            for (int terminados = 0; terminados < n;) {
                siguiente = agregarLlegadas(listos, siguiente, reloj);
                if (listos.isEmpty()) { reloj = avanzarAProximaLlegada(siguiente); continue; }
                int actual = listos.remove(); iniciar(actual);
                ejecutar(actual, restante[actual]); terminados++;
            }
        }

        void sjf() {
            PriorityQueue<Integer> listos = new PriorityQueue<>(porSjf());
            int siguiente = 0;
            for (int terminados = 0; terminados < n;) {
                siguiente = agregarLlegadas(listos, siguiente, reloj);
                if (listos.isEmpty()) { reloj = avanzarAProximaLlegada(siguiente); continue; }
                int actual = listos.remove(); iniciar(actual);
                ejecutar(actual, restante[actual]); terminados++;
            }
        }

        void srtf() {
            PriorityQueue<Integer> listos = new PriorityQueue<>(porSrtf());
            int siguiente = 0, actual = -1, terminados = 0;
            while (terminados < n) {
                siguiente = agregarLlegadas(listos, siguiente, reloj);
                if (actual < 0) {
                    if (listos.isEmpty()) { reloj = avanzarAProximaLlegada(siguiente); continue; }
                    actual = listos.remove(); iniciar(actual);
                } else if (!listos.isEmpty() && restante[listos.peek()] < restante[actual]) {
                    listos.add(actual); actual = listos.remove(); iniciar(actual);
                }
                long duracion = restante[actual];
                if (siguiente < n) {
                    long hastaLlegada = procesos.get(llegadas.get(siguiente)).llegada() - reloj;
                    if (hastaLlegada > 0) duracion = Math.min(duracion, hastaLlegada);
                }
                ejecutar(actual, duracion);
                if (restante[actual] == 0) { actual = -1; terminados++; }
            }
        }

        void rr(long quantum) {
            ArrayDeque<Integer> cola = new ArrayDeque<>(); int siguiente = 0, terminados = 0;
            while (terminados < n) {
                while (siguiente < n && procesos.get(llegadas.get(siguiente)).llegada() <= reloj) cola.addLast(llegadas.get(siguiente++));
                if (cola.isEmpty()) { reloj = avanzarAProximaLlegada(siguiente); continue; }
                int actual = cola.removeFirst(); iniciar(actual);
                long duracion = Math.min(restante[actual], quantum);
                if (cola.isEmpty() && siguiente < n) {
                    long hastaProxima = procesos.get(llegadas.get(siguiente)).llegada() - reloj;
                    if (hastaProxima >= restante[actual]) duracion = restante[actual];
                } else if (cola.isEmpty() && siguiente == n) duracion = restante[actual];
                ejecutar(actual, duracion);
                while (siguiente < n && procesos.get(llegadas.get(siguiente)).llegada() <= reloj) cola.addLast(llegadas.get(siguiente++));
                if (restante[actual] == 0) terminados++; else cola.addLast(actual);
            }
        }

        private int agregarLlegadas(PriorityQueue<Integer> cola, int siguiente, long hasta) {
            while (siguiente < n && procesos.get(llegadas.get(siguiente)).llegada() <= hasta) cola.add(llegadas.get(siguiente++));
            return siguiente;
        }
        private long avanzarAProximaLlegada(int siguiente) {
            if (siguiente >= n) throw new IllegalStateException("Quedan procesos sin llegada pendiente.");
            long proxima = procesos.get(llegadas.get(siguiente)).llegada();
            if (proxima > reloj) agregarSegmento(-1, reloj, proxima);
            return proxima;
        }
        private void iniciar(int i) { if (primerInicio[i] < 0) primerInicio[i] = reloj; }
        private void ejecutar(int i, long duracion) {
            long finTurno = Math.addExact(reloj, duracion);
            agregarSegmento(procesos.get(i).id(), reloj, finTurno);
            restante[i] -= duracion; reloj = finTurno;
            if (restante[i] == 0) fin[i] = reloj;
        }
        private void agregarSegmento(int pid, long inicio, long finSegmento) {
            if (finSegmento <= inicio) return;
            boolean ocioso = pid < 0;
            int pidSegmento = ocioso ? 0 : pid;
            if (!segmentos.isEmpty()) {
                Segmento ultimo = segmentos.get(segmentos.size() - 1);
                if (ultimo.fin() == inicio && ultimo.ocioso() == ocioso && (ocioso || ultimo.procesoId() == pid)) {
                    segmentos.set(segmentos.size() - 1, new Segmento(pidSegmento, ultimo.inicio(), finSegmento, ocioso)); return;
                }
            }
            segmentos.add(new Segmento(pidSegmento, inicio, finSegmento, ocioso));
        }
        Resultado resultado() {
            List<Metrica> metricas = new ArrayList<>(n);
            for (int i = 0; i < n; i++) {
                Proceso p = procesos.get(i);
                long retorno = Math.subtractExact(fin[i], p.llegada());
                metricas.add(new Metrica(p.id(), Math.subtractExact(retorno, p.rafaga()), retorno,
                        Math.subtractExact(primerInicio[i], p.llegada())));
            }
            return new Resultado(List.copyOf(segmentos), List.copyOf(metricas));
        }
    }
}
