FROM maven:3.9.9-eclipse-temurin-21
WORKDIR /workspace
RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev && \
    rm -rf /var/lib/apt/lists/*
COPY backend/planificador-service/pom.xml ./pom.xml
COPY backend/planificador-service/src ./src
COPY native/include ./native/include
COPY native/src ./native/src
RUN mvn -B -DskipTests compile dependency:build-classpath \
    -Dmdep.outputFile=/workspace/dependencies.classpath && \
    gcc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fPIC -shared \
    -Inative/include native/src/planificador.c -o /workspace/libplanificador.so
COPY tests/benchmarks/ejecutar-contenedor.sh /usr/local/bin/ejecutar-contenedor
RUN chmod +x /usr/local/bin/ejecutar-contenedor
ENV PLANIFICADOR_BIBLIOTECA=/workspace/libplanificador.so
ENTRYPOINT ["/usr/local/bin/ejecutar-contenedor"]
