FROM maven:3.9.9-eclipse-temurin-21 AS build
WORKDIR /workspace
COPY backend/proceso-service/pom.xml ./pom.xml
COPY backend/proceso-service/src ./src
RUN mvn -B package

FROM eclipse-temurin:21-jre-jammy
WORKDIR /app
COPY --from=build /workspace/target/proceso-service-0.0.1-SNAPSHOT.jar app.jar
COPY infra/swarm/proceso-entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh
EXPOSE 8082
ENTRYPOINT ["/app/entrypoint.sh"]
