import { TestBed } from '@angular/core/testing';
import { HttpClient, provideHttpClient, withInterceptors } from '@angular/common/http';
import { HttpTestingController, provideHttpClientTesting } from '@angular/common/http/testing';
import { firstValueFrom } from 'rxjs';
import { vi } from 'vitest';
import { AuthService, jwtInterceptor } from './auth.service';
import { PlanificadorApi, Planificacion, SolicitudPlanificacion } from './planificador-api.service';
import { appSettings } from './app.settings';

describe('PlanificadorApi', () => {
  let api: PlanificadorApi;
  let http: HttpTestingController;
  const registro: Planificacion = {
    algorithmId: 'rr', planificationId: 'corrida-1', createdAt: '2026-11-14T12:00:00Z',
    processes: [{ pid: 1, arrivalTime: 0, burstTime: 2, priority: 0 }],
    segments: [{ kind: 'execution', pid: 1, start: 0, end: 2 }],
    processMetrics: [{ pid: 1, arrivalTime: 0, burstTime: 2, completionTime: 2, turnaroundTime: 2, waitingTime: 0, responseTime: 0 }],
    averages: { waitingTime: 0, turnaroundTime: 2, responseTime: 0 },
  };
  beforeEach(() => {
    TestBed.configureTestingModule({ providers: [
      provideHttpClient(withInterceptors([jwtInterceptor])), provideHttpClientTesting(),
      { provide: AuthService, useValue: { token: vi.fn().mockResolvedValue('token-de-prueba') } },
    ] });
    api = TestBed.inject(PlanificadorApi); http = TestBed.inject(HttpTestingController);
  });
  afterEach(() => http.verify());

  it('consulta el catalogo dinamico con JWT, sin filtrar algoritmos futuros', async () => {
    const pendiente = firstValueFrom(api.algoritmos()); await Promise.resolve();
    const request = http.expectOne(appSettings.apiUrl + '/algoritmos');
    expect(request.request.method).toBe('GET');
    expect(request.request.headers.get('Authorization')).toBe('Bearer token-de-prueba');
    const respuesta = { algorithms: [{ id: 'fcfs' }, { id: 'algoritmo-futuro' }] };
    request.flush(respuesta); expect(await pendiente).toEqual(respuesta);
  });

  it('envia el cuerpo exacto de planificacion y el quantum', async () => {
    const cuerpo: SolicitudPlanificacion = { algorithmId: 'rr', processes: registro.processes, quantum: 3 };
    const pendiente = firstValueFrom(api.ejecutar(cuerpo)); await Promise.resolve();
    const request = http.expectOne(appSettings.apiUrl + '/planificaciones');
    expect(request.request.method).toBe('POST'); expect(request.request.body).toEqual(cuerpo);
    request.flush(registro, { status: 201, statusText: 'Created' });
    expect(await pendiente).toEqual(registro);
  });

  it('lee lista y detalle del historial respetando las estructuras del contrato', async () => {
    const lista = firstValueFrom(api.historial()); await Promise.resolve();
    http.expectOne(appSettings.apiUrl + '/historial').flush({ items: [registro] });
    expect((await lista).items).toEqual([registro]);
    const detalle = firstValueFrom(api.detalle(registro.planificationId)); await Promise.resolve();
    http.expectOne(appSettings.apiUrl + '/historial/corrida-1').flush(registro);
    expect(await detalle).toEqual(registro);
  });

  it('no envia el JWT a otros origenes', async () => {
    const pendiente = firstValueFrom(TestBed.inject(HttpClient).get('https://otro.example/datos'));
    const request = http.expectOne('https://otro.example/datos');
    expect(request.request.headers.has('Authorization')).toBe(false);
    request.flush({}); await pendiente;
    expect(TestBed.inject(AuthService).token).not.toHaveBeenCalled();
  });
});
