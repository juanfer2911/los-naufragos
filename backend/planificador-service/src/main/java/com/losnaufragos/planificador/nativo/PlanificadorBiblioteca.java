package com.losnaufragos.planificador.nativo;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.ptr.IntByReference;

public interface PlanificadorBiblioteca extends Library {
    int planificador_ejecutar_v1(String algoritmoId, Pointer procesos, int cantidad,
            long quantum, Pointer segmentos, int capacidadSegmentos,
            IntByReference segmentosRequeridos, Pointer metricas, int capacidadMetricas,
            IntByReference metricasRequeridas);

    Pointer listar_algoritmos(IntByReference cantidad);

    @Structure.FieldOrder({"id", "llegada", "rafaga", "prioridad"})
    class Proceso extends Structure {
        public int id;
        public long llegada;
        public long rafaga;
        public int prioridad;
    }

    @Structure.FieldOrder({"procesoId", "inicio", "fin", "esOcioso"})
    class Segmento extends Structure {
        public int procesoId;
        public long inicio;
        public long fin;
        public int esOcioso;
    }

    @Structure.FieldOrder({"procesoId", "espera", "retorno", "respuesta"})
    class Metrica extends Structure {
        public int procesoId;
        public long espera;
        public long retorno;
        public long respuesta;
    }

    @Structure.FieldOrder({"id", "nombre", "requiereQuantum"})
    class Algoritmo extends Structure {
        public byte[] id = new byte[16];
        public byte[] nombre = new byte[64];
        public int requiereQuantum;

        public Algoritmo() { }

        public Algoritmo(Pointer memoria) {
            super(memoria);
            read();
        }
    }
}
