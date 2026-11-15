// app.ts
import { Component, computed, inject, OnInit, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { firstValueFrom } from 'rxjs';
import { HttpErrorResponse } from '@angular/common/http';
import { AuthService } from './auth.service';
import { Planificacion, PlanificadorApi } from './planificador-api.service';

export interface Proceso {
  pid: number;
  llegada: number;
  rafaga: number;
  prioridad: number;
}

export interface RebanadaGantt {
  kind: 'execution' | 'idle';
  pid?: number;
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
export class App implements OnInit {
  readonly auth = inject(AuthService);
  private readonly api = inject(PlanificadorApi);
  vistaActual = signal<'inicio' | 'configuracion' | 'resultados' | 'conjuntos' | 'historial'>('inicio');
  sesionIniciada = this.auth.sesionIniciada;
  usuario = this.auth.usuario;
  correo = '';
  cargando = signal(false);
  error = signal('');
  historial = signal<Planificacion[]>([]);
  resultados = signal<Record<string, Planificacion>>({});

  // SEÑAL PARA EL ALGORITMO SELECCIONADO
  algoritmoSeleccionado = signal('');

  procesos = signal<Proceso[]>([
    { pid: 1, llegada: 0, rafaga: 8, prioridad: 3 },
    { pid: 2, llegada: 2, rafaga: 4, prioridad: 1 },
    { pid: 3, llegada: 4, rafaga: 9, prioridad: 2 },
    { pid: 4, llegada: 5, rafaga: 5, prioridad: 4 }
  ]);

  quantum = signal<number | null>(3);
  algoritmosDisponibles = signal<{ id: string }[]>([]);
  algoritmosActivos = signal<string[]>([]);
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

  resultadoActual = computed(() => this.resultados()[this.algoritmoSeleccionado()]);
  rebanadasGantt = computed<RebanadaGantt[]>(() => (this.resultadoActual()?.segments ?? []).map(s => ({
    kind: s.kind, pid: s.kind === 'execution' ? s.pid : undefined,
    inicio: s.start, fin: s.end, color: s.kind === 'idle' ? '#94a3b8' : this.colorProceso(s.pid),
  })));
  metricas = computed<MetricaProceso[]>(() => (this.resultadoActual()?.processMetrics ?? []).map(m => ({
    pid: m.pid, llegada: m.arrivalTime, rafaga: m.burstTime, fin: m.completionTime,
    retorno: m.turnaroundTime, espera: m.waitingTime, respuesta: m.responseTime,
  })));
  promedioEspera = computed(() => this.resultadoActual()?.averages.waitingTime ?? 0);
  promedioRetorno = computed(() => this.resultadoActual()?.averages.turnaroundTime ?? 0);
  promedioRespuesta = computed(() => this.resultadoActual()?.averages.responseTime ?? 0);
  algoritmoGanador = computed(() => {
    const resultados = Object.values(this.resultados()).map(r => ({ algoritmo: r.algorithmId, espera: r.averages.waitingTime }));
    return resultados.sort((a, b) => a.espera - b.espera)[0] ?? null;
  });

  ngOnInit() { if (this.sesionIniciada()) void this.cargarDatos(); }

  async cargarDatos() {
    this.cargando.set(true); this.error.set('');
    try {
      const [catalogo, historial] = await Promise.all([
        firstValueFrom(this.api.algoritmos()), firstValueFrom(this.api.historial()),
      ]);
      this.algoritmosDisponibles.set(catalogo.algorithms);
      this.algoritmosActivos.set(catalogo.algorithms.map(a => a.id));
      this.algoritmoSeleccionado.set(catalogo.algorithms[0]?.id ?? '');
      this.historial.set(historial.items);
    } catch (error) { this.mostrarError(error); }
    finally { this.cargando.set(false); }
  }

  cambiarVista(nuevaVista: 'inicio' | 'configuracion' | 'resultados' | 'conjuntos' | 'historial') {
    this.vistaActual.set(nuevaVista);
    if (nuevaVista === 'historial') void this.cargarHistorial();
  }

  cambiarAlgoritmo(alg: string) {
    this.algoritmoSeleccionado.set(alg);
  }

  nombreAlgoritmo(id: string) { return id === 'rr' ? 'Round Robin' : id === 'fcfs' ? 'FIFO' : id.toUpperCase(); }

  async ejecutarSimulacion() {
    if (this.cargando()) return;
    if (!this.algoritmosActivos().length || !this.procesos().length) {
      this.error.set('Agrega procesos y selecciona al menos un algoritmo.'); return;
    }
    if (this.algoritmosActivos().includes('rr') && (!Number.isSafeInteger(this.quantum()) || (this.quantum() ?? 0) < 1)) {
      this.error.set('Indica un quantum entero mayor o igual que 1.'); return;
    }
    if (this.procesos().some(p => ![p.pid, p.llegada, p.rafaga, p.prioridad].every(Number.isSafeInteger)
        || p.pid < 0 || p.llegada < 0 || p.rafaga < 1)) {
      this.error.set('Los procesos requieren enteros validos: llegada no negativa y rafaga positiva.'); return;
    }
    this.cargando.set(true); this.error.set(''); this.resultados.set({});
    try {
      // Cada POST guarda una corrida; las metricas y promedios vienen de C/backend.
      for (const id of this.algoritmosActivos()) {
        const resultado = await firstValueFrom(this.api.ejecutar({
          algorithmId: id,
          processes: this.procesos().map(p => ({ pid: p.pid, arrivalTime: p.llegada, burstTime: p.rafaga, priority: p.prioridad })),
          ...(id === 'rr' ? { quantum: this.quantum()! } : {}),
        }));
        this.resultados.update(anteriores => ({ ...anteriores, [id]: resultado }));
      }
      this.algoritmoSeleccionado.set(this.algoritmosActivos()[0]);
      this.vistaActual.set('resultados');
      this.historial.set((await firstValueFrom(this.api.historial())).items);
    } catch (error) { this.mostrarError(error); }
    finally { this.cargando.set(false); }
  }

  async cargarHistorial() {
    if (this.cargando()) return;
    this.cargando.set(true); this.error.set('');
    try { this.historial.set((await firstValueFrom(this.api.historial())).items); }
    catch (error) { this.mostrarError(error); }
    finally { this.cargando.set(false); }
  }

  async abrirPlanificacion(id: string) {
    if (this.cargando()) return;
    this.cargando.set(true); this.error.set('');
    try {
      const resultado = await firstValueFrom(this.api.detalle(id));
      this.procesos.set(resultado.processes.map(p => ({ pid: p.pid, llegada: p.arrivalTime, rafaga: p.burstTime, prioridad: p.priority })));
      this.resultados.set({ [resultado.algorithmId]: resultado });
      this.algoritmoSeleccionado.set(resultado.algorithmId);
      this.algoritmosActivos.set([resultado.algorithmId]);
      // El contrato HTTP no publica el quantum historico; no inventar su valor.
      if (resultado.algorithmId === 'rr') this.quantum.set(null);
      this.vistaActual.set('resultados');
    } catch (error) { this.mostrarError(error); }
    finally { this.cargando.set(false); }
  }

  private mostrarError(error: unknown) {
    this.error.set(error instanceof HttpErrorResponse
      ? error.status === 401 ? 'Sesion no valida. Inicia sesion nuevamente.' : error.error?.message ?? 'No se pudo conectar con el backend.'
      : error instanceof Error ? error.message : 'No se pudo completar la operacion.');
  }

  eliminarProceso(pid: number) {
    this.procesos.update(lista => lista.filter(p => p.pid !== pid));
    this.resultados.set({});
  }

  iniciarSesion() { return this.auth.iniciarSesion(this.correo); }
  cerrarSesion() { return this.auth.cerrarSesion(); }
  exportarInforme() { window.print(); }
  colorProceso(pid: number) { return ['#2b84ea', '#3b82f6', '#f59e0b', '#8b5cf6', '#14b8a6', '#ec4899'][(Math.max(1, pid) - 1) % 6]; }
  alternarAlgoritmo(alg: string, activo: boolean) {
    this.algoritmosActivos.update(lista => activo ? [...new Set([...lista, alg])] : lista.filter(item => item !== alg));
    if (this.algoritmosActivos().length && !this.algoritmosActivos().includes(this.algoritmoSeleccionado())) {
      this.cambiarAlgoritmo(this.algoritmosActivos()[0]);
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
    this.resultados.set({});
  }
  cargarConjunto(id: number) {
    const conjunto = this.conjuntos().find(item => item.id === id);
    if (conjunto) this.procesos.set(conjunto.procesos.map(p => ({ ...p })));
    this.resultados.set({});
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

}
