FROM maven:3.9.9-eclipse-temurin-21 AS build
WORKDIR /workspace
COPY backend/planificador-service/pom.xml ./pom.xml
COPY backend/planificador-service/src ./src
RUN mvn -B -DskipTests package
COPY native/include ./native/include
COPY native/src ./native/src
RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev && \
    gcc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fPIC -shared \
    -Inative/include native/src/planificador.c -o /workspace/libplanificador.so && \
    rm -rf /var/lib/apt/lists/*

FROM eclipse-temurin:21-jre-jammy
WORKDIR /app
COPY --from=build /workspace/target/planificador-service-0.0.1-SNAPSHOT.jar app.jar
COPY --from=build /workspace/libplanificador.so /app/libplanificador.so
COPY infra/swarm/planificador-entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh
ENV PLANIFICADOR_BIBLIOTECA=/app/libplanificador.so
EXPOSE 8081
ENTRYPOINT ["/app/entrypoint.sh"]
