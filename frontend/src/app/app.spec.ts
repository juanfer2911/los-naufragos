import { TestBed } from '@angular/core/testing';
import { App } from './app';
import { signal } from '@angular/core';
import { provideHttpClient } from '@angular/common/http';
import { HttpTestingController, provideHttpClientTesting } from '@angular/common/http/testing';
import { AuthService } from './auth.service';
import { Planificacion } from './planificador-api.service';
import { vi } from 'vitest';

describe('App', () => {
  beforeEach(async () => {
    window.history.replaceState({}, '', '/');
    await TestBed.configureTestingModule({
      imports: [App],
      providers: [provideHttpClient(), provideHttpClientTesting(), {
        provide: AuthService,
        useValue: { sesionIniciada: signal(false), usuario: signal(''), error: signal(''), esDocente: signal(false),
          iniciarSesion: vi.fn(), cerrarSesion: vi.fn() },
      }],
    })
      .compileComponents();
  });

  it('should create the app', () => {
    const fixture = TestBed.createComponent(App);
    const app = fixture.componentInstance;
    expect(app).toBeTruthy();
  });

  it('muestra el formulario de acceso al simulador', async () => {
    const fixture = TestBed.createComponent(App);
    await fixture.whenStable();
    const compiled = fixture.nativeElement as HTMLElement;
    expect(compiled.querySelector('h1')?.textContent).toContain('Iniciar Sesión');
    expect(compiled.querySelector('input[name="correo"]')).not.toBeNull();
    expect(compiled.querySelector('input[type="password"]')).toBeNull();
    expect(compiled.querySelector('button[type="submit"]')?.textContent).toContain('Acceder con Keycloak');
  });

  it('conserva la ruta protegida y solicita login al abrirla sin sesión', async () => {
    window.history.replaceState({}, '', '/historial');
    const fixture = TestBed.createComponent(App);
    fixture.detectChanges();
    await fixture.whenStable();
    expect(fixture.componentInstance.vistaActual()).toBe('historial');
    expect(TestBed.inject(AuthService).iniciarSesion).toHaveBeenCalled();
    expect(window.location.pathname).toBe('/historial');
  });

  it('sincroniza la vista con la URL al navegar', () => {
    const fixture = TestBed.createComponent(App);
    fixture.detectChanges();
    const app = fixture.componentInstance;
    app.cambiarVista('resultados');
    expect(window.location.pathname).toBe('/resultados');
    expect(app.vistaActual()).toBe('resultados');
  });

  it('compara las tres métricas y muestra todos los algoritmos empatados', () => {
    const app = TestBed.createComponent(App).componentInstance;
    const fila = (algorithmId: string, waitingTime: number, turnaroundTime: number, responseTime: number): Planificacion => ({
      algorithmId, planificationId: `id-${algorithmId}`, createdAt: '2026-11-18T00:00:00Z',
      processes: [], segments: [], processMetrics: [],
      averages: { waitingTime, turnaroundTime, responseTime },
    });
    app.algoritmosDisponibles.set(['fcfs', 'sjf', 'srtf', 'rr'].map(id => ({ id })));
    app.resultados.set({
      fcfs: fila('fcfs', 1, 5, 2), sjf: fila('sjf', 1, 3, 2),
      srtf: fila('srtf', 2, 3, 1), rr: fila('rr', 3, 5, 3),
    });
    expect(app.comparacionPromedios()).toHaveLength(4);
    expect(app.ganadoresComparacion()).toEqual({ espera: ['fcfs', 'sjf'], retorno: ['sjf', 'srtf'], respuesta: ['srtf'] });
  });

  it('representa la ociosidad por kind y conserva los valores del backend', () => {
    const app = TestBed.createComponent(App).componentInstance;
    const registro: Planificacion = {
      algorithmId: 'fcfs', planificationId: 'corrida-1', createdAt: '2026-11-14T12:00:00Z',
      processes: [{ pid: 0, arrivalTime: 2, burstTime: 1, priority: 0 }],
      segments: [{ kind: 'idle', start: 0, end: 2 }, { kind: 'execution', pid: 0, start: 2, end: 3 }],
      processMetrics: [{ pid: 0, arrivalTime: 2, burstTime: 1, completionTime: 3, turnaroundTime: 1, waitingTime: 0, responseTime: 0 }],
      averages: { waitingTime: 0, turnaroundTime: 1, responseTime: 0 },
    };
    app.algoritmoSeleccionado.set('fcfs'); app.resultados.set({ fcfs: registro });
    expect(app.rebanadasGantt()[0].kind).toBe('idle');
    expect(app.rebanadasGantt()[1].kind).toBe('execution');
    expect(app.rebanadasGantt()[1].pid).toBe(0);
    expect(app.metricas()[0].fin).toBe(3);
    expect(app.promedioRetorno()).toBe(1);
  });

  it('oculta escritura para estudiante y muestra creacion y eliminacion para docente', () => {
    const fixture = TestBed.createComponent(App); fixture.detectChanges();
    const app = fixture.componentInstance;
    app.sesionIniciada.set(true); app.vistaActual.set('conjuntos');
    app.conjuntos.set([{ conjuntoId: 'compartido', name: 'Banco', processes: [] }]);
    fixture.detectChanges();
    expect(fixture.nativeElement.textContent).not.toContain('Crear Nuevo Banco');
    expect(fixture.nativeElement.querySelector('.danger-link')).toBeNull();
    app.auth.esDocente.set(true); fixture.detectChanges();
    expect(fixture.nativeElement.textContent).toContain('Crear Nuevo Banco');
    expect(fixture.nativeElement.querySelector('.danger-link')).not.toBeNull();
    expect(fixture.nativeElement.textContent).not.toContain('Editar Banco');
  });

  it('rechaza escritura local de estudiante sin enviar HTTP', async () => {
    const app = TestBed.createComponent(App).componentInstance;
    app.nombreConjunto = 'No autorizado';
    await app.guardarConjunto(); await app.eliminarConjunto('id');
    TestBed.inject(HttpTestingController).expectNone(() => true);
  });

  it('carga detalle real y envia procesos completos, quantum y sourceSetId al planificador', async () => {
    const app = TestBed.createComponent(App).componentInstance;
    const http = TestBed.inject(HttpTestingController);
    const processes = [{ pid: 3, arrivalTime: 1, burstTime: 4, priority: 0 }];
    const cargado = app.cargarConjunto('compartido');
    http.expectOne('http://localhost:8082/conjuntos/compartido').flush({ conjuntoId: 'compartido', name: 'Banco', processes, quantum: 8 });
    await cargado;
    expect(app.procesos()).toEqual([{ pid: 3, llegada: 1, rafaga: 4, prioridad: 0 }]);
    expect(app.vistaActual()).toBe('configuracion');
    expect(app.quantum()).toBe(8);
    app.algoritmosActivos.set(['rr']); app.quantum.set(5);
    const ejecutado = app.ejecutarSimulacion();
    const post = http.expectOne('http://localhost:8081/planificaciones');
    expect(post.request.body).toEqual({ algorithmId: 'rr', processes, quantum: 5, sourceSetId: 'compartido' });
    post.flush({ algorithmId: 'rr', processes, segments: [], processMetrics: [], averages: { waitingTime: 0, turnaroundTime: 4, responseTime: 0 } });
    await Promise.resolve(); await Promise.resolve();
    http.expectOne('http://localhost:8081/historial').flush({ items: [] });
    await ejecutado; http.verify();
    expect(app.vistaActual()).toBe('resultados');
  });

  it('guarda el quantum configurado como dato del conjunto', async () => {
    const app = TestBed.createComponent(App).componentInstance;
    const http = TestBed.inject(HttpTestingController);
    app.auth.esDocente.set(true); app.nombreConjunto = 'Quantum propio'; app.quantum.set(8);
    const guardado = app.guardarConjunto();
    const post = http.expectOne('http://localhost:8082/conjuntos');
    expect(post.request.body.quantum).toBe(8);
    post.flush({ ...post.request.body, conjuntoId: 'nuevo' });
    await guardado;
    expect(app.conjuntos()[0].quantum).toBe(8); http.verify();
  });

  it('restablece 3 al cargar un conjunto antiguo sin quantum', async () => {
    const app = TestBed.createComponent(App).componentInstance;
    const http = TestBed.inject(HttpTestingController);
    app.quantum.set(8);
    const cargado = app.cargarConjunto('antiguo');
    http.expectOne('http://localhost:8082/conjuntos/antiguo').flush({ conjuntoId: 'antiguo', name: 'Antiguo', processes: [] });
    await cargado; expect(app.quantum()).toBe(3); http.verify();
  });
});
