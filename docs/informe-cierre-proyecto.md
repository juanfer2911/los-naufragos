# Informe de cierre del proyecto

**Proyecto:** Los Náufragos — Planificador de procesos
**Fecha:** 17 de noviembre de 2026

## Resumen

Se desarrolló una aplicación web para registrar procesos, guardar y reutilizar conjuntos de prueba, ejecutar algoritmos de planificación y consultar resultados e historial. La solución integra Angular, dos microservicios Spring Boot, un núcleo de planificación en C invocado desde Java mediante JNA, PostgreSQL y autenticación con Keycloak.

El proyecto implementa los ocho requisitos funcionales definidos para el planificador. La interfaz permite administrar las entradas y presentar los resultados; los servicios validan y procesan las solicitudes, y la persistencia conserva los conjuntos y las corridas.

## Funcionalidades implementadas

| Requisito funcional | Solución implementada |
|---|---|
| **RF-01. Registrar y persistir procesos** | Captura procesos con PID, llegada, ráfaga y prioridad. Los conjuntos y sus procesos se almacenan en PostgreSQL con operaciones transaccionales. |
| **RF-02. Guardar y recuperar conjuntos** | Creación, listado, consulta y carga de conjuntos con nombre y procesos. Angular permite reutilizar los datos guardados; el quantum del conjunto es opcional. |
| **RF-03. Ejecutar algoritmos y mostrar resultados** | FCFS/FIFO, SJF, SRTF y Round Robin se ejecutan desde el servicio de planificación. La respuesta incluye segmentos de ejecución/ocio y métricas por proceso. |
| **RF-04. Configurar quantum para Round Robin** | El quantum se puede definir para cada corrida y se transmite al motor sin recompilar. También se puede conservar como dato opcional del conjunto. |
| **RF-05. Presentar el diagrama de Gantt** | Angular representa los segmentos en orden temporal, identifica los procesos y distingue los periodos de CPU ociosa con sus marcas de tiempo. |
| **RF-06. Calcular métricas y promedios** | El backend devuelve espera, retorno y respuesta por proceso, además de los promedios de cada métrica. Angular presenta los valores recibidos. |
| **RF-07. Comparar algoritmos** | Se pueden ejecutar varios algoritmos con la misma carga. La vista comparativa muestra los promedios de espera, retorno y respuesta, e identifica el menor valor de cada métrica, incluidos los empates. |
| **RF-08. Validar entradas** | La interfaz y los servicios validan los datos de procesos y quantum. Las API responden con errores JSON; los mensajes de validación identifican el campo y la regla aplicable. |

## Arquitectura y operación

- Angular consume directamente los dos servicios; la solución conserva la arquitectura de exactamente dos microservicios y no incorpora un API Gateway.
- `proceso-service` administra los conjuntos compartidos y su persistencia.
- `planificador-service` expone el catálogo, ejecuta las corridas e incluye el historial asociado al usuario autenticado.
- El motor integra la biblioteca C mediante JNA y ofrece un motor Java seleccionable por configuración.
- Keycloak proporciona autenticación OIDC con PKCE. Los servicios validan los JWT y aplican las reglas de autorización por rol y propietario.
- Docker Compose permite iniciar la infraestructura local. Docker Swarm integra frontend, microservicios, PostgreSQL y Keycloak para la demostración.

## Avances y resultados

- Se implementaron y probaron los cuatro algoritmos de planificación en C, junto con su integración desde Java mediante JNA.
- Se desarrollaron los dos servicios backend y la interfaz Angular, conectados a PostgreSQL y Keycloak.
- Se verificaron los flujos de inicio de sesión, consulta del catálogo, ejecución de algoritmos, gestión de conjuntos, consulta del historial y permisos por rol en el entorno integrado.
- Se desplegó el sistema completo en Docker Swarm con cinco servicios activos.
- Se compararon los motores C/JNA y Java en los bancos de rendimiento. Para las cargas medidas, los resultados coincidieron en segmentos y métricas.

### Prueba de Round Robin con dos valores de quantum

Se ejecutó la misma carga con quantum 2 y 5. Los procesos fueron P1 (llegada 0, ráfaga 8), P2 (llegada 2, ráfaga 4), P3 (llegada 4, ráfaga 9) y P4 (llegada 5, ráfaga 5). Las dos ejecuciones produjeron secuencias de Gantt diferentes.

| Quantum | Espera promedio | Retorno promedio | Respuesta promedio |
|---:|---:|---:|---:|
| 2 | 10,5 ms | 17,0 ms | 1,8 ms |
| 5 | 9,8 ms | 16,3 ms | 4,3 ms |

La prueba muestra el efecto del quantum en la planificación: con quantum 2 mejoró el tiempo medio de respuesta; con quantum 5 fueron menores los promedios de espera y retorno.

## Cierre

La solución integra captura y reutilización de procesos, planificación con cuatro algoritmos, presentación y comparación de métricas, persistencia, historial y control de acceso. Con estas funcionalidades, el proyecto cubre los requisitos funcionales del planificador mediante una aplicación web integrada y desplegable.
