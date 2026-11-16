FROM postgres:16.4
COPY infra/postgres/init/01-create-databases.sql /docker-entrypoint-initdb.d/01-create-databases.sql
COPY infra/swarm/postgres/entrypoint.sh /usr/local/bin/naufragos-entrypoint.sh
RUN chmod +x /usr/local/bin/naufragos-entrypoint.sh
ENTRYPOINT ["/usr/local/bin/naufragos-entrypoint.sh"]
CMD ["postgres"]
