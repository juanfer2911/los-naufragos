// app.ts
import { Component, computed, inject, OnInit, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { firstValueFrom } from 'rxjs';
import { HttpErrorResponse } from '@angular/common/http';
import { AuthService } from './auth.service';
import { Planificacion, PlanificadorApi } from './planificador-api.service';
import { Conjunto, ProcesoApi } from './proceso-api.service';

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
  private readonly conjuntosApi = inject(ProcesoApi);
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
  conjuntos = signal<Conjunto[]>([]);
  conjuntoSeleccionado = signal('');
  sourceSetId = signal<string | undefined>(undefined);
  cargandoConjuntos = signal(false);
  mostrarFormularioConjunto = signal(false);
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
    if (nuevaVista === 'conjuntos' || nuevaVista === 'configuracion') void this.cargarConjuntos();
  }

  cambiarAlgoritmo(alg: string) {
    this.algoritmoSeleccionado.set(alg);
  }

  nombreAlgoritmo(id: string) { return id === 'rr' ? 'Round Robin' : id === 'fcfs' ? 'FIFO' : id.toUpperCase(); }

  async ejecutarSimulacion() {
    if (this.cargando() || this.cargandoConjuntos()) return;
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
          ...(this.sourceSetId() ? { sourceSetId: this.sourceSetId() } : {}),
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
      this.sourceSetId.set(resultado.sourceSetId);
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
  async cargarConjuntos() {
    if (this.cargandoConjuntos()) return;
    this.cargandoConjuntos.set(true); this.error.set('');
    try {
      const lista = await firstValueFrom(this.conjuntosApi.listar());
      this.conjuntos.set(lista.items);
      if (!lista.items.some(c => c.conjuntoId === this.conjuntoSeleccionado())) {
        this.conjuntoSeleccionado.set(lista.items[0]?.conjuntoId ?? '');
      }
    } catch (error) { this.mostrarError(error); }
    finally { this.cargandoConjuntos.set(false); }
  }

  async cargarConjunto(id: string) {
    if (!id || this.cargandoConjuntos()) return;
    this.cargandoConjuntos.set(true); this.error.set('');
    try {
      const conjunto = await firstValueFrom(this.conjuntosApi.detalle(id));
      this.quantum.set(conjunto.quantum ?? 3);
      this.procesos.set(conjunto.processes.map(p => ({ pid: p.pid, llegada: p.arrivalTime, rafaga: p.burstTime, prioridad: p.priority })));
      this.sourceSetId.set(conjunto.conjuntoId);
      this.conjuntoSeleccionado.set(conjunto.conjuntoId);
      this.resultados.set({});
      this.mostrarFormularioConjunto.set(false);
      this.vistaActual.set('configuracion');
    } catch (error) { this.mostrarError(error); }
    finally { this.cargandoConjuntos.set(false); }
  }

  async guardarConjunto() {
    if (!this.auth.esDocente() || this.cargandoConjuntos() || !this.nombreConjunto.trim()) return;
    if (this.quantum() !== null && (!Number.isSafeInteger(this.quantum()) || this.quantum()! < 1)) {
      this.error.set('Indica un quantum entero mayor o igual que 1.'); return;
    }
    this.cargandoConjuntos.set(true); this.error.set('');
    try {
      const creado = await firstValueFrom(this.conjuntosApi.crear({
        name: this.nombreConjunto.trim(), description: this.descripcionConjunto.trim(),
        ...(this.quantum() !== null ? { quantum: this.quantum()! } : {}),
        processes: this.procesos().map(p => ({ pid: p.pid, arrivalTime: p.llegada, burstTime: p.rafaga, priority: p.prioridad })),
      }));
      this.conjuntos.update(lista => [...lista, creado]);
      this.conjuntoSeleccionado.set(creado.conjuntoId);
      this.sourceSetId.set(creado.conjuntoId);
      this.cancelarEdicionConjunto();
    } catch (error) { this.mostrarError(error); }
    finally { this.cargandoConjuntos.set(false); }
  }
  nuevoConjunto() {
    if (!this.auth.esDocente()) return;
    this.cancelarEdicionConjunto(); this.mostrarFormularioConjunto.set(true);
  }
  cancelarEdicionConjunto() { this.mostrarFormularioConjunto.set(false); this.nombreConjunto = ''; this.descripcionConjunto = ''; }
  async eliminarConjunto(id: string) {
    if (!this.auth.esDocente() || this.cargandoConjuntos()) return;
    if (!window.confirm('Eliminar este conjunto compartido?')) return;
    this.cargandoConjuntos.set(true); this.error.set('');
    try {
      await firstValueFrom(this.conjuntosApi.eliminar(id));
      this.conjuntos.update(lista => lista.filter(c => c.conjuntoId !== id));
      if (this.sourceSetId() === id) this.sourceSetId.set(undefined);
      if (this.conjuntoSeleccionado() === id) this.conjuntoSeleccionado.set(this.conjuntos()[0]?.conjuntoId ?? '');
    } catch (error) { this.mostrarError(error); }
    finally { this.cargandoConjuntos.set(false); }
  }

  private procesoVacio(): Proceso { return { pid: 0, llegada: 0, rafaga: 1, prioridad: 1 }; }

}
