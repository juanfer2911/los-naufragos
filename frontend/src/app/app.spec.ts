import { TestBed } from '@angular/core/testing';
import { App } from './app';
import { signal } from '@angular/core';
import { provideHttpClient } from '@angular/common/http';
import { provideHttpClientTesting } from '@angular/common/http/testing';
import { AuthService } from './auth.service';
import { Planificacion } from './planificador-api.service';
import { vi } from 'vitest';

describe('App', () => {
  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [App],
      providers: [provideHttpClient(), provideHttpClientTesting(), {
        provide: AuthService,
        useValue: { sesionIniciada: signal(false), usuario: signal(''), error: signal(''),
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
});
