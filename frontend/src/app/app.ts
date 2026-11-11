// app.ts
import { Component, computed, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

export interface Proceso {
  pid: number;
  llegada: number;
  rafaga: number;
  prioridad: number;
}

export interface RebanadaGantt {
  pid: number;
  inicio: number;
  fin: number;
  color: string;
}

export interface MetricaProceso {
  pid: number;
  llegada: number;
  rafaga: number;
  fin: number;
  retorno: number;
  espera: number;
  respuesta: number;
}

export interface ConjuntoPrueba {
  id: number;
  nombre: string;
  descripcion: string;
  procesos: Proceso[];
  fecha: string;
}

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class App {
  vistaActual = signal<'inicio' | 'configuracion' | 'resultados' | 'conjuntos'>('configuracion');
  sesionIniciada = signal(false);
  modoRegistro = signal(false);
  usuario = 'Juan Pérez';
  correo = '';
  contrasena = '';

  // SEÑAL PARA EL ALGORITMO SELECCIONADO
  algoritmoSeleccionado = signal<'SJF' | 'FIFO' | 'SRTF' | 'RR'>('SJF');

  procesos = signal<Proceso[]>([
    { pid: 1, llegada: 0, rafaga: 8, prioridad: 3 },
    { pid: 2, llegada: 2, rafaga: 4, prioridad: 1 },
    { pid: 3, llegada: 4, rafaga: 9, prioridad: 2 },
    { pid: 4, llegada: 5, rafaga: 5, prioridad: 4 }
  ]);

  quantum = signal(3);
  algoritmosDisponibles = ['FIFO', 'SJF', 'SRTF', 'RR'];
  algoritmosActivos = signal<string[]>(['FIFO', 'SJF', 'SRTF', 'RR']);
  borradorProceso: Proceso = this.procesoVacio();
  procesoEditando = signal<number | null>(null);
  mostrarFormularioProceso = signal(false);
  conjuntos = signal<ConjuntoPrueba[]>([
    { id: 1, nombre: 'Prueba 1 - Caso Base', descripcion: 'Escenario estándar de 4 procesos para comparar rendimiento.', fecha: '12/03/2026', procesos: this.clonarProcesosIniciales() },
    { id: 2, nombre: 'Prueba 2 - Round Robin Extremo', descripcion: 'Varios procesos con ráfagas cortas y quantum reducido.', fecha: '10/03/2026', procesos: [{ pid: 1, llegada: 0, rafaga: 1, prioridad: 3 }, { pid: 2, llegada: 0, rafaga: 2, prioridad: 2 }, { pid: 3, llegada: 1, rafaga: 1, prioridad: 4 }, { pid: 4, llegada: 2, rafaga: 2, prioridad: 1 }, { pid: 5, llegada: 3, rafaga: 1, prioridad: 2 }] },
    { id: 3, nombre: 'Prueba 3 - Carga Intensa', descripcion: 'Prioridades invertidas para probar inanición.', fecha: '05/03/2026', procesos: [{ pid: 1, llegada: 0, rafaga: 7, prioridad: 4 }, { pid: 2, llegada: 1, rafaga: 4, prioridad: 3 }, { pid: 3, llegada: 2, rafaga: 3, prioridad: 2 }, { pid: 4, llegada: 3, rafaga: 2, prioridad: 1 }] },
    { id: 4, nombre: 'Prueba 4 - Caso Monotónico', descripcion: 'Procesos que llegan en orden secuencial.', fecha: '28/02/2026', procesos: [{ pid: 1, llegada: 0, rafaga: 5, prioridad: 1 }, { pid: 2, llegada: 5, rafaga: 5, prioridad: 2 }, { pid: 3, llegada: 10, rafaga: 5, prioridad: 3 }, { pid: 4, llegada: 15, rafaga: 5, prioridad: 4 }] }
  ]);
  conjuntoSeleccionado = signal(1);
  conjuntoEditando = signal<number | null>(null);
  nombreConjunto = '';
  descripcionConjunto = '';

  private simulacion = computed(() => this.simular(this.algoritmoSeleccionado()));
  rebanadasGantt = computed(() => this.simulacion().rebanadas);
  metricas = computed(() => this.simulacion().metricas);
  promedioEspera = computed(() => this.promedio('espera'));
  promedioRetorno = computed(() => this.promedio('retorno'));
  promedioRespuesta = computed(() => this.promedio('respuesta'));
  promedioRafaga = computed(() => this.promedio('rafaga'));
  algoritmoGanador = computed(() => {
    const resultados = this.algoritmosActivos().map(algoritmo => {
      const metricas = this.simular(algoritmo).metricas;
      const espera = metricas.length ? metricas.reduce((total, metrica) => total + metrica.espera, 0) / metricas.length : Number.POSITIVE_INFINITY;
      return { algoritmo, espera };
    });
    return resultados.sort((a, b) => a.espera - b.espera)[0] ?? null;
  });

  cambiarVista(nuevaVista: 'inicio' | 'configuracion' | 'resultados' | 'conjuntos') {
    this.vistaActual.set(nuevaVista);
  }

  cambiarAlgoritmo(alg: 'SJF' | 'FIFO' | 'SRTF' | 'RR') {
    this.algoritmoSeleccionado.set(alg);
  }

  eliminarProceso(pid: number) {
    this.procesos.update(lista => lista.filter(p => p.pid !== pid));
  }

  iniciarSesion() { this.sesionIniciada.set(true); this.vistaActual.set('inicio'); }
  cerrarSesion() { this.sesionIniciada.set(false); this.vistaActual.set('configuracion'); }
  exportarInforme() { window.print(); }
  alternarModoRegistro() { this.modoRegistro.update(valor => !valor); }
  colorProceso(pid: number) { return ['#2b84ea', '#3b82f6', '#f59e0b', '#8b5cf6', '#14b8a6', '#ec4899'][(Math.max(1, pid) - 1) % 6]; }
  alternarAlgoritmo(alg: string, activo: boolean) {
    this.algoritmosActivos.update(lista => activo ? [...new Set([...lista, alg])] : lista.filter(item => item !== alg));
    if (this.algoritmosActivos().length && !this.algoritmosActivos().includes(this.algoritmoSeleccionado())) {
      this.cambiarAlgoritmo(this.algoritmosActivos()[0] as 'SJF' | 'FIFO' | 'SRTF' | 'RR');
    }
  }
  abrirNuevoProceso() { this.procesoEditando.set(null); this.borradorProceso = this.procesoVacio(); this.mostrarFormularioProceso.set(true); }
  editarProceso(proceso: Proceso) { this.procesoEditando.set(proceso.pid); this.borradorProceso = { ...proceso }; this.mostrarFormularioProceso.set(true); }
  guardarProceso() {
    if (!this.borradorProceso.rafaga || this.borradorProceso.rafaga < 1 || this.borradorProceso.llegada < 0) return;
    const editando = this.procesoEditando();
    if (editando === null) {
      const pid = Math.max(0, ...this.procesos().map(p => p.pid)) + 1;
      this.procesos.update(lista => [...lista, { ...this.borradorProceso, pid }]);
    } else {
      this.procesos.update(lista => lista.map(p => p.pid === editando ? { ...this.borradorProceso, pid: editando } : p));
    }
    this.mostrarFormularioProceso.set(false);
  }
  cargarConjunto(id: number) {
    const conjunto = this.conjuntos().find(item => item.id === id);
    if (conjunto) this.procesos.set(conjunto.procesos.map(p => ({ ...p })));
  }
  guardarConjunto() {
    if (!this.nombreConjunto.trim()) return;
    const editando = this.conjuntoEditando();
    const conjunto = { id: editando ?? Math.max(0, ...this.conjuntos().map(item => item.id)) + 1, nombre: this.nombreConjunto.trim(), descripcion: this.descripcionConjunto.trim() || 'Conjunto creado por el usuario.', fecha: new Date().toLocaleDateString('es-CO'), procesos: this.procesos().map(p => ({ ...p })) };
    this.conjuntos.update(lista => editando === null ? [...lista, conjunto] : lista.map(item => item.id === editando ? conjunto : item));
    this.cancelarEdicionConjunto();
  }
  editarConjunto(conjunto: ConjuntoPrueba) { this.conjuntoEditando.set(conjunto.id); this.nombreConjunto = conjunto.nombre; this.descripcionConjunto = conjunto.descripcion; this.procesos.set(conjunto.procesos.map(p => ({ ...p }))); }
  cancelarEdicionConjunto() { this.conjuntoEditando.set(null); this.nombreConjunto = ''; this.descripcionConjunto = ''; }
  eliminarConjunto(id: number) { this.conjuntos.update(lista => lista.filter(item => item.id !== id)); }

  private procesoVacio(): Proceso { return { pid: 0, llegada: 0, rafaga: 1, prioridad: 1 }; }
  private clonarProcesosIniciales(): Proceso[] { return [{ pid: 1, llegada: 0, rafaga: 8, prioridad: 3 }, { pid: 2, llegada: 2, rafaga: 4, prioridad: 1 }, { pid: 3, llegada: 4, rafaga: 9, prioridad: 2 }, { pid: 4, llegada: 5, rafaga: 5, prioridad: 4 }]; }

  private promedio(campo: keyof MetricaProceso): number {
    const lista = this.metricas();
    return lista.length ? lista.reduce((total, m) => total + Number(m[campo]), 0) / lista.length : 0;
  }

  private simular(algoritmo: string): { rebanadas: RebanadaGantt[]; metricas: MetricaProceso[] } {
    const procesos = [...this.procesos()].sort((a, b) => a.llegada - b.llegada || a.pid - b.pid);
    if (!procesos.length) return { rebanadas: [], metricas: [] };
    const restantes = new Map(procesos.map(p => [p.pid, p.rafaga]));
    const primerInicio = new Map<number, number>();
    const fin = new Map<number, number>();
    const tramos: Array<{ pid: number; inicio: number; fin: number }> = [];
    let tiempo = 0;
    const segmento = (pid: number, inicio: number, final: number) => {
      const ultimo = tramos[tramos.length - 1];
      if (ultimo && ultimo.pid === pid && ultimo.fin === inicio) ultimo.fin = final;
      else tramos.push({ pid, inicio, fin: final });
    };
    if (algoritmo === 'FIFO' || algoritmo === 'SJF') {
      const pendientes = [...procesos];
      while (pendientes.length) {
        const disponibles = pendientes.filter(p => p.llegada <= tiempo);
        if (!disponibles.length) { const siguiente = Math.min(...pendientes.map(p => p.llegada)); segmento(-1, tiempo, siguiente); tiempo = siguiente; continue; }
        const p = (algoritmo === 'SJF' ? disponibles.sort((a, b) => a.rafaga - b.rafaga || a.llegada - b.llegada || a.pid - b.pid) : disponibles.sort((a, b) => a.llegada - b.llegada || a.pid - b.pid))[0];
        pendientes.splice(pendientes.findIndex(item => item.pid === p.pid), 1); primerInicio.set(p.pid, tiempo); segmento(p.pid, tiempo, tiempo + p.rafaga); tiempo += p.rafaga; fin.set(p.pid, tiempo);
      }
    } else if (algoritmo === 'RR') {
      const pendientes = [...procesos]; const cola: Proceso[] = []; const quantum = Math.max(1, this.quantum());
      while (pendientes.length || cola.length) {
        while (pendientes.length && pendientes[0].llegada <= tiempo) cola.push(pendientes.shift()!);
        if (!cola.length) { const siguiente = pendientes[0].llegada; segmento(-1, tiempo, siguiente); tiempo = siguiente; continue; }
        const p = cola.shift()!; if (!primerInicio.has(p.pid)) primerInicio.set(p.pid, tiempo);
        const ejecutar = Math.min(quantum, restantes.get(p.pid)!);
        segmento(p.pid, tiempo, tiempo + ejecutar);
        for (let tick = 0; tick < ejecutar; tick++) { tiempo++; restantes.set(p.pid, restantes.get(p.pid)! - 1); while (pendientes.length && pendientes[0].llegada <= tiempo) cola.push(pendientes.shift()!); }
        if (restantes.get(p.pid) === 0) fin.set(p.pid, tiempo); else cola.push(p);
      }
    } else {
      while (procesos.some(p => !fin.has(p.pid))) {
        const disponibles = procesos.filter(p => p.llegada <= tiempo && !fin.has(p.pid));
        if (!disponibles.length) { const siguiente = Math.min(...procesos.filter(p => !fin.has(p.pid)).map(p => p.llegada)); segmento(-1, tiempo, siguiente); tiempo = siguiente; continue; }
        const p = disponibles.sort((a, b) => restantes.get(a.pid)! - restantes.get(b.pid)! || a.llegada - b.llegada || a.pid - b.pid)[0];
        if (!primerInicio.has(p.pid)) primerInicio.set(p.pid, tiempo);
        segmento(p.pid, tiempo, tiempo + 1); restantes.set(p.pid, restantes.get(p.pid)! - 1); tiempo++;
        if (restantes.get(p.pid) === 0) fin.set(p.pid, tiempo);
      }
    }
    const colores = ['#2b84ea', '#3b82f6', '#f59e0b', '#8b5cf6', '#14b8a6', '#ec4899'];
    const rebanadas = tramos.map(t => ({ ...t, color: t.pid === -1 ? '#94a3b8' : colores[(Math.max(1, t.pid) - 1) % colores.length] }));
    const metricas = procesos.map(p => { const final = fin.get(p.pid) ?? 0; const respuesta = (primerInicio.get(p.pid) ?? p.llegada) - p.llegada; const retorno = final - p.llegada; return { pid: p.pid, llegada: p.llegada, rafaga: p.rafaga, fin: final, retorno, espera: retorno - p.rafaga, respuesta }; });
    return { rebanadas, metricas };
  }
}
