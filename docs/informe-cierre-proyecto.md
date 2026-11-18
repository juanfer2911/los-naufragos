# Informe de cierre del proyecto

**Proyecto:** Los Náufragos — Planificador de procesos
**Fecha del informe:** 17 de noviembre de 2026
**Alcance:** estado del código y evidencias documentadas en este repositorio.

## Resumen

Se construyó una aplicación web para registrar procesos, guardarlos como conjuntos reutilizables, ejecutar algoritmos de planificación y consultar sus segmentos, métricas e historial. La solución integra una interfaz Angular, dos servicios Spring Boot, el núcleo de planificación en C llamado desde Java mediante JNA, PostgreSQL y autenticación/autorización con Keycloak.

Los ocho requisitos funcionales tienen una implementación en el sistema. La matriz de este informe distingue esa implementación de la aceptación final: ciertos criterios específicos requieren todavía una prueba o evidencia formal. Por ello, el cierre funcional se sustenta en el comportamiento existente y en las pruebas registradas, sin atribuir como realizadas las verificaciones pendientes.

## Solución entregada

- **Planificación:** FCFS/FIFO, SJF, SRTF y Round Robin. El servicio de planificación expone un catálogo de algoritmos y recibe procesos y quantum cuando corresponde.
- **Resultados:** segmentos de ejecución y ocio, métricas por proceso, promedios de espera, retorno y respuesta, y una interfaz para comparar algoritmos ejecutados con la misma entrada.
- **Conjuntos de prueba:** creación, consulta, carga y eliminación de conjuntos persistidos; el quantum de un conjunto es opcional.
- **Historial:** las corridas se guardan y se consultan por el propietario identificado en el JWT.
- **Seguridad:** Keycloak con OIDC y PKCE; los servicios validan JWT. La consulta de conjuntos está disponible para usuarios autenticados; la escritura requiere rol docente.
- **Despliegue:** infraestructura local con Docker Compose y stack de demostración Docker Swarm, con PostgreSQL, Keycloak, frontend y ambos servicios.

La arquitectura conserva exactamente dos microservicios y Angular se comunica directamente con ellos. La API y la interfaz no sustituyen la planificación: los resultados y promedios se producen en el backend a partir de los motores C/JNA y Java.

## Matriz de requisitos funcionales

| Requisito | Implementación entregada | Estado de aceptación según evidencia disponible |
|---|---|---|
| **RF-01. Registrar y persistir procesos** | Captura de procesos con PID, llegada, ráfaga y prioridad; persistencia transaccional de conjuntos y procesos en PostgreSQL. | **Implementado.** Se documentan persistencia y pruebas de servicio. Falta registrar el ensayo académico concreto de cinco procesos tras reiniciar los contenedores. |
| **RF-02. Guardar y recuperar conjuntos** | Conjuntos con nombre y procesos; endpoints de creación, listado y detalle; Angular permite guardarlos y cargarlos. | **Implementado y verificado** en pruebas del servicio, Angular y recorridos documentados con PostgreSQL/Keycloak. |
| **RF-03. Ejecutar algoritmos y mostrar resultados** | Angular envía la misma entrada al algoritmo seleccionado; el servicio usa C/JNA por defecto y devuelve segmentos y métricas. | **Implementado.** Hay pruebas de algoritmos y recorrido real documentado. Falta adjuntar el conjunto oficial resuelto manualmente y su aceptación formal para cerrar ese criterio específico. |
| **RF-04. Configurar quantum de Round Robin** | Quantum editable por corrida y opcional al guardar un conjunto; se envía al backend sin recompilar. | **Implementado.** Se documenta ejecución real de RR con quantum. Falta registrar la comparación exigida de la misma entrada con quantum 2 y 5, incluyendo sus resultados. |
| **RF-05. Mostrar diagrama de Gantt** | La interfaz representa segmentos, marcas de tiempo y periodos de CPU ociosa. | **Implementado.** Falta documentar la aceptación visual exacta de ocho procesos a 1366 px y comprobar que todas las etiquetas sean legibles. |
| **RF-06. Mostrar métricas y promedios** | Se muestran espera, retorno y respuesta por proceso y sus promedios; los promedios llegan desde el backend. | **Implementado y respaldado por pruebas** de C/JNA, servicio y recorrido documentado de Angular. |
| **RF-07. Comparar algoritmos** | La pantalla presenta una tabla de promedios para los algoritmos ejecutados y señala el mínimo de cada métrica, incluidos empates. | **Implementado en la interfaz.** El código actual calcula los ganadores por espera, retorno y respuesta. La corrección reciente del mensaje de espera que muestra empates aún requiere compilación y verificación visual. |
| **RF-08. Validar entradas** | Angular valida llegada no negativa, ráfaga positiva, quantum positivo y enteros; los servicios aplican validación HTTP y responden errores estructurados. El formulario de proceso muestra mensajes para valores inválidos. | **Implementado.** Queda revisar que los errores del backend identifiquen claramente el campo y el valor requerido en todos los casos. |

## Evidencias registradas

La documentación del repositorio registra pruebas unitarias de Angular y ambos servicios, pruebas nativas C, integración Java/JNA, recorridos con PostgreSQL y Keycloak reales, verificación de roles y consultas de historial. También registra el despliegue Swarm estable, el rechazo de peticiones anónimas y la configuración de tokens de corta duración. Los bancos C/JNA frente a Java reportan igualdad exacta de segmentos y métricas para las muestras medidas.

Las fuentes principales son [auditoría de requisitos](auditoria-requisitos.md), [evidencias de seguridad y usabilidad](evidencias-seguridad-usabilidad.md), [guía de infraestructura](../infra/README.md) y los README de los servicios y frontend. Las pruebas citadas son las anotadas en esos documentos; este informe no afirma que se hayan vuelto a ejecutar al redactarlo.

## Cierre

El proyecto cuenta con una solución funcional integrada que cubre las capacidades centrales solicitadas: capturar y reutilizar cargas, ejecutar los algoritmos, consultar resultados y métricas, comparar corridas, proteger el acceso y conservar datos e historial. Por tanto, los requisitos funcionales están implementados en el producto.

Para declarar una **aceptación académica completa**, deben cerrarse y archivarse los ensayos específicos que aún aparecen pendientes en la matriz: persistencia después del reinicio de contenedores, oráculo oficial de FIFO, comparación de quantum 2/5, aceptación visual del Gantt y revisión de mensajes de validación. También debe verificarse visualmente la modificación reciente de empates en la comparación. No se presentan estas evidencias como completadas.
