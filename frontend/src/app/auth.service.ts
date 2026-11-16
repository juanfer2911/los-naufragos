import { inject, Injectable, signal } from '@angular/core';
import { HttpInterceptorFn } from '@angular/common/http';
import { from, switchMap } from 'rxjs';
import Keycloak from 'keycloak-js';
import { appSettings } from './app.settings';

@Injectable({ providedIn: 'root' })
export class AuthService {
  readonly sesionIniciada = signal(false);
  readonly usuario = signal('');
  readonly error = signal('');
  readonly esDocente = signal(false);
  private readonly keycloak = new Keycloak(appSettings.keycloak);

  async inicializar(): Promise<void> {
    this.keycloak.onAuthSuccess = () => this.actualizarRol();
    this.keycloak.onAuthRefreshSuccess = () => this.actualizarRol();
    this.keycloak.onAuthLogout = () => { this.sesionIniciada.set(false); this.esDocente.set(false); };
    this.keycloak.onAuthRefreshError = () => { this.sesionIniciada.set(false); this.esDocente.set(false); };
    this.keycloak.onTokenExpired = () => { void this.token().catch(() => {}); };
    try {
      const autenticado = await this.keycloak.init({
        onLoad: 'check-sso',
        flow: 'standard',
        pkceMethod: 'S256',
        responseMode: 'query',
        checkLoginIframe: false,
        redirectUri: window.location.origin + '/',
      });
      this.sesionIniciada.set(autenticado);
      this.actualizarRol();
      const perfil = this.keycloak.tokenParsed;
      this.usuario.set(String(perfil?.['name'] ?? perfil?.['preferred_username'] ?? 'Usuario'));
    } catch {
      this.error.set('No se pudo conectar con Keycloak. Comprueba el servidor y vuelve a cargar la pagina.');
    }
  }

  async iniciarSesion(loginHint: string): Promise<void> {
    this.error.set('');
    try {
      await this.keycloak.login({ redirectUri: window.location.origin + '/', loginHint: loginHint.trim() || undefined });
    } catch {
      this.error.set('No se pudo iniciar la autenticacion con Keycloak.');
    }
  }

  async cerrarSesion(): Promise<void> {
    try {
      await this.keycloak.logout({ redirectUri: window.location.origin + '/' });
    } catch {
      this.error.set('No se pudo cerrar la sesion de Keycloak. Intenta nuevamente.');
    }
  }

  async token(): Promise<string> {
    if (!this.sesionIniciada()) throw new Error('Inicia sesion con Keycloak.');
    try {
      await this.keycloak.updateToken(30);
      if (!this.keycloak.token) throw new Error('No hay un access token disponible.');
      return this.keycloak.token;
    } catch {
      this.keycloak.clearToken();
      this.sesionIniciada.set(false);
      this.esDocente.set(false);
      this.error.set('Tu sesion ha vencido. Inicia sesion nuevamente.');
      throw new Error('La sesion ha vencido.');
    }
  }

  private actualizarRol() {
    this.esDocente.set(this.keycloak.authenticated === true && this.keycloak.hasRealmRole('docente'));
  }
}

// Solo adjuntar el access token a nuestro backend, nunca a otros destinos.
export const jwtInterceptor: HttpInterceptorFn = (request, next) => {
  if (![appSettings.apiUrl, appSettings.procesoApiUrl].some(url => request.url.startsWith(url + '/'))) return next(request);
  return from(inject(AuthService).token()).pipe(
    switchMap(token => next(request.clone({ setHeaders: { Authorization: `Bearer ${token}` } }))),
  );
};
