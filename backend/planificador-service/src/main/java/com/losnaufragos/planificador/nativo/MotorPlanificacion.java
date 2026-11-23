package com.losnaufragos.planificador.nativo;

import java.util.List;

import static com.losnaufragos.planificador.nativo.PlanificadorNativo.Algoritmo;
import static com.losnaufragos.planificador.nativo.PlanificadorNativo.Proceso;
import static com.losnaufragos.planificador.nativo.PlanificadorNativo.Resultado;

/** Interfaz común para la biblioteca C/JNA y la implementación Java de referencia. */
public interface MotorPlanificacion {
    List<Algoritmo> listarAlgoritmos();
    Resultado ejecutar(String algoritmoId, List<Proceso> procesos, long quantum);
}
