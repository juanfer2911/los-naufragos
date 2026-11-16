import { inject, Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { appSettings } from './app.settings';
import { ProcesoApi as Proceso } from './planificador-api.service';

export interface ConjuntoInput {
  name: string;
  description?: string;
  quantum?: number;
  processes: Proceso[];
}
export interface Conjunto extends ConjuntoInput { conjuntoId: string; }

@Injectable({ providedIn: 'root' })
export class ProcesoApi {
  private readonly http = inject(HttpClient);
  private readonly url = appSettings.procesoApiUrl + '/conjuntos';
  listar() { return this.http.get<{ items: Conjunto[] }>(this.url); }
  detalle(id: string) { return this.http.get<Conjunto>(`${this.url}/${encodeURIComponent(id)}`); }
  crear(entrada: ConjuntoInput) { return this.http.post<Conjunto>(this.url, entrada); }
  eliminar(id: string) { return this.http.delete<void>(`${this.url}/${encodeURIComponent(id)}`); }
}
