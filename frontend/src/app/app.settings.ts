// Configuracion publica de desarrollo; nunca incluir secretos ni tokens.
export const appSettings = {
  apiUrl: 'http://localhost:8081',
  keycloak: {
    url: 'http://localhost:8080',
    realm: 'planificador-realm',
    clientId: 'angular-desarrollo',
  },
};
