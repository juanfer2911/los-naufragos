FROM quay.io/keycloak/keycloak:25.0.6
ENV KC_DB=postgres
COPY backend/planificador-service/dev/keycloak-realm.json /opt/keycloak/data/import/planificador-realm.json
RUN /opt/keycloak/bin/kc.sh build
COPY --chmod=755 infra/swarm/keycloak/entrypoint.sh /opt/keycloak/bin/naufragos-entrypoint.sh
ENTRYPOINT ["/opt/keycloak/bin/naufragos-entrypoint.sh"]
