# planificador-service

Estructura inicial del microservicio de planificación. Todavía no incluye endpoints de planificación, algoritmos Java, JNA, seguridad ni persistencia.

## Requisitos

- Java 21
- Maven 3.6.3 o superior

## Probar

Desde la raíz del repositorio:

```sh
mvn -f backend/planificador-service/pom.xml test
```

## Ejecutar

Desde la raíz del repositorio:

```sh
mvn -f backend/planificador-service/pom.xml spring-boot:run
```

Spring Boot inicia el servidor web en el puerto predeterminado 8080. Aún no hay rutas HTTP implementadas.
