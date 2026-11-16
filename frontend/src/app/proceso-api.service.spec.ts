import { TestBed } from '@angular/core/testing';
import { provideHttpClient, withInterceptors } from '@angular/common/http';
import { HttpTestingController, provideHttpClientTesting } from '@angular/common/http/testing';
import { firstValueFrom } from 'rxjs';
import { vi } from 'vitest';
import { AuthService, jwtInterceptor } from './auth.service';
import { ProcesoApi } from './proceso-api.service';
import { appSettings } from './app.settings';

describe('ProcesoApi', () => {
  const conjunto = { conjuntoId: 'id/1', name: 'Compartido', quantum: 8, processes: [{ pid: 1, arrivalTime: 0, burstTime: 2, priority: 0 }] };
  let api: ProcesoApi;
  let http: HttpTestingController;
  beforeEach(() => {
    TestBed.configureTestingModule({ providers: [provideHttpClient(withInterceptors([jwtInterceptor])), provideHttpClientTesting(),
      { provide: AuthService, useValue: { token: vi.fn().mockResolvedValue('jwt-prueba') } }] });
    api = TestBed.inject(ProcesoApi); http = TestBed.inject(HttpTestingController);
  });
  afterEach(() => http.verify());
  it('consulta lista y detalle con JWT y escapa el identificador', async () => {
    const lista = firstValueFrom(api.listar()); await Promise.resolve();
    const r = http.expectOne(appSettings.procesoApiUrl + '/conjuntos');
    expect(r.request.headers.get('Authorization')).toBe('Bearer jwt-prueba');
    r.flush({ items: [conjunto] }); expect((await lista).items).toEqual([conjunto]);
    const detalle = firstValueFrom(api.detalle(conjunto.conjuntoId)); await Promise.resolve();
    http.expectOne(appSettings.procesoApiUrl + '/conjuntos/id%2F1').flush(conjunto);
    expect(await detalle).toEqual(conjunto);
  });
  it('crea con el JSON exacto del contrato y elimina sin cuerpo', async () => {
    const { conjuntoId, ...entrada } = conjunto;
    const creado = firstValueFrom(api.crear(entrada)); await Promise.resolve();
    const post = http.expectOne(appSettings.procesoApiUrl + '/conjuntos');
    expect(post.request.method).toBe('POST'); expect(post.request.body).toEqual(entrada);
    post.flush(conjunto, { status: 201, statusText: 'Created' }); await creado;
    const eliminado = firstValueFrom(api.eliminar(conjuntoId)); await Promise.resolve();
    const del = http.expectOne(appSettings.procesoApiUrl + '/conjuntos/id%2F1');
    expect(del.request.method).toBe('DELETE');
    del.flush(null, { status: 204, statusText: 'No Content' }); await eliminado;
  });
});
