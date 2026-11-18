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

## Evidencias registradas

El repositorio documenta pruebas nativas de los cuatro algoritmos, pruebas unitarias e integración de los servicios, pruebas de Angular y recorridos con PostgreSQL y Keycloak reales. También registra la persistencia de conjuntos, el control de roles, la consulta privada del historial, el despliegue del stack Swarm y los resultados de los bancos C/JNA frente a Java, que comprobaron la igualdad de segmentos y métricas para las cargas medidas.

Las evidencias y los procedimientos se describen en [auditoría de requisitos](auditoria-requisitos.md), [evidencias de seguridad y usabilidad](evidencias-seguridad-usabilidad.md), [infraestructura](../infra/README.md), y los README de [planificador-service](../backend/planificador-service/README.md), [proceso-service](../backend/proceso-service/README.md) y [frontend](../frontend/README.md). Este informe resume los registros existentes y no representa una nueva ejecución de esas pruebas.

## Cierre

La solución integra captura y reutilización de procesos, planificación con cuatro algoritmos, presentación y comparación de métricas, persistencia, historial y control de acceso. Con estas funcionalidades, el proyecto cubre los requisitos funcionales del planificador mediante una aplicación web integrada y desplegable.
