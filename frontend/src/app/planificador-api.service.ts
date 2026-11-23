import { inject, Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { appSettings } from './app.settings';

export interface ProcesoApi {
  pid: number;
  arrivalTime: number;
  burstTime: number;
  priority: number;
}
export interface SolicitudPlanificacion {
  algorithmId: string;
  processes: ProcesoApi[];
  quantum?: number;
  sourceSetId?: string;
}
export type SegmentoApi =
  | { kind: 'execution'; pid: number; start: number; end: number }
  | { kind: 'idle'; start: number; end: number };
export interface MetricaApi {
  pid: number;
  arrivalTime: number;
  burstTime: number;
  completionTime: number;
  turnaroundTime: number;
  waitingTime: number;
  responseTime: number;
}
export interface Planificacion {
  algorithmId: string;
  segments: SegmentoApi[];
  processMetrics: MetricaApi[];
  averages: { waitingTime: number; turnaroundTime: number; responseTime: number };
  planificationId: string;
  createdAt: string;
  processes: ProcesoApi[];
  sourceSetId?: string;
}

@Injectable({ providedIn: 'root' })
export class PlanificadorApi {
  private readonly http = inject(HttpClient);
  private readonly url = appSettings.apiUrl;

  algoritmos() { return this.http.get<{ algorithms: { id: string }[] }>(`${this.url}/algoritmos`); }
  ejecutar(solicitud: SolicitudPlanificacion) { return this.http.post<Planificacion>(`${this.url}/planificaciones`, solicitud); }
  historial() { return this.http.get<{ items: Planificacion[] }>(`${this.url}/historial`); }
  detalle(id: string) { return this.http.get<Planificacion>(`${this.url}/historial/${encodeURIComponent(id)}`); }
}
