# planificador-service

Spring Boot 3.5.7 y Java 21, con Java → JNA → C, OAuth2 Resource Server JWT y almacenamiento JDBC en PostgreSQL. Implementa `GET /algoritmos`, `POST /planificaciones`, `GET /historial` y `GET /historial/{planificacionId}` según OpenAPI. C se carga al consultar el catálogo o ejecutar una planificación.

Consulta la [guía REST, Keycloak y PostgreSQL](docs/ejecucion-rest.md) para iniciar el servicio y conectar Angular. La integración con PostgreSQL 16.4 y Keycloak 25.0.6 reales quedó [verificada en Docker Desktop y WSL](docs/verificacion-real.md), incluida la recuperación de cuatro planificaciones después de reiniciar Java. La suite actual de 19 pruebas conserva su entorno H2/JWKS de pruebas e incluye la conservación interna del quantum sin cambios en el JSON público.

El cierre de etapa aplicó ese cambio al backend de 8081 y confirmó en PostgreSQL real una nueva corrida RR con quantum 3. Los cuatro registros históricos se conservaron intactos; la evidencia del cierre está enlazada en la guía de verificación real.

## Requisitos

- JDK 21 Linux, Maven y GCC/Make en Ubuntu/WSL.
- Biblioteca y JVM de arquitectura compatible. ABI comprobada: Linux x86-64.

## Probar

Desde la raíz del repositorio, en una terminal Ubuntu/WSL:

```sh
java -version
mvn -version
make -C native test
make -C native library
mvn -f backend/planificador-service/pom.xml \
  -Dplanificador.biblioteca="$PWD/native/libplanificador.so" test
```

Esto ejecuta 19 pruebas: una de contexto, nueve de integración nativa y nueve HTTP con Tomcat real, H2 y JWT RSA/JWKS local. Sin `-Dplanificador.biblioteca`, las pruebas nativas y HTTP se omiten explícitamente; un `mvn test` sin esa propiedad no demuestra esas integraciones. Con la propiedad y una JVM Windows fallan con un mensaje que requiere Linux.

La ruta debe ser absoluta y apuntar a la biblioteca generada en `native/`; no se copia a otro directorio.

En esta sesión se descargaron herramientas Linux en `target/herramientas/`, ignoradas por Git. Para reproducir usando esas herramientas locales, desde la raíz en WSL:

```sh
export JAVA_HOME="$PWD/backend/planificador-service/target/herramientas/jdk-21.0.13+9"
export PATH="$JAVA_HOME/bin:$PWD/backend/planificador-service/target/herramientas/apache-maven-3.9.11/bin:$PATH"
make -C native library
mvn -f backend/planificador-service/pom.xml \
  -Dmaven.repo.local=/mnt/c/Users/juanf/.m2/repository \
  -Dplanificador.biblioteca="$PWD/native/libplanificador.so" test
```

Estas herramientas son temporales: `mvn clean` elimina `target/`. Para uso habitual configura JDK 21 y Maven Linux fuera de la salida de compilación; la integración no depende de esas descargas ni de esa caché Maven particular.

## IntelliJ IDEA

Usa un JDK 21 de Ubuntu/WSL para ejecutar `PlanificadorNativoTests`, con las dependencias Maven recargadas. En las opciones de la JVM configura:

```text
-Dplanificador.biblioteca=/mnt/c/Users/juanf/Documents/los-naufragos/native/libplanificador.so
```

Puedes usar el JDK Linux descargado en `target/herramientas/jdk-21.0.13+9` para esta sesión. Si la configuración de ejecución usa el JDK de Windows, cámbiala a WSL. Como alternativa, ejecuta los comandos anteriores desde la terminal Ubuntu integrada en IntelliJ. La prueba de catálogo también ejecuta todos los identificadores obtenidos desde C.

## Uso del adaptador

```java
var motor = PlanificadorNativo.desdeConfiguracion();
var catalogo = motor.listarAlgoritmos();
var resultado = motor.ejecutar(catalogo.getFirst().id(),
        List.of(new PlanificadorNativo.Proceso(1, 0, 5, 0)), 0);
```

Importa `com.losnaufragos.planificador.nativo.PlanificadorNativo` y `java.util.List`. La configuración acepta la propiedad JVM `planificador.biblioteca` o la variable `PLANIFICADOR_BIBLIOTECA`; la propiedad tiene precedencia. La activación de las pruebas usa la propiedad JVM.

`PlanificadorBiblioteca` mapea estructuras y funciones. `PlanificadorNativo` escribe procesos contiguos, sondea cantidades, reserva buffers y devuelve listas inmutables de segmentos y métricas. Traduce estados 1–5 a mensajes comprensibles; no recalcula la planificación. Los tiempos son `long` Java y los contadores `int`. Las tres estructuras originales ocupan 32 bytes.

`listar_algoritmos()` devuelve memoria estática de solo lectura del catálogo C: id ASCII de 16 bytes, nombre de 64 y un entero de 32 bits `requiere_quantum` (84 bytes en total). Java copia el contenido a sus propios registros, sin liberar memoria C ni mantener un catálogo independiente. `GET /algoritmos` expone únicamente los identificadores, como define OpenAPI.

Los segmentos ociosos se identifican por `ocioso`, no por su PID: C devuelve actualmente PID 0 para esos intervalos.

## Iniciar Spring

Desde la raíz del repositorio:

```sh
mvn -f backend/planificador-service/pom.xml spring-boot:run
```

La aplicación usa el puerto 8081 para no colisionar con Keycloak (8080). Antes de iniciar hay que configurar base, emisor JWT y ruta de la biblioteca siguiendo la guía REST. Las pruebas Maven usan H2 y no modifican PostgreSQL.
