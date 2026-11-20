# Anexos y sustentacion - indice de entrega

Corte: 2026-11-19. Este indice distingue evidencia ya disponible de evidencia que solo puede producirse durante actividades externas o la demostracion ante el docente.

| Anexo | Evidencia | Estado |
|---|---|---|
| A. Controles funcionales | [Matriz C1-C6](controles-manuales.md) y [captura Gantt 8 procesos a 1366 px](evidencia-gantt-8-procesos-1366.png) | Ejecutados en la interfaz; la UI autoasigna PID. Cotejar el control oficial si el curso lo entrega. |
| B. Seguridad | [Evidencias de seguridad y usabilidad](evidencias-seguridad-usabilidad.md) | 403/401 y renovacion proactiva verificados. Expiracion real y sesiones U1-U3 pendientes. |
| C. Usabilidad RQ-03 | Tabla anonima U1-U3 y guion en [evidencias-seguridad-usabilidad.md](evidencias-seguridad-usabilidad.md) | Pendiente de tres participantes ajenos al equipo; no se reemplaza con automatizacion. |
| D. Benchmarks | [Protocolos y resultados Web 2/Arquitectura](../tests/benchmarks/README.md) | Mediciones locales y Swarm disponibles; protocolos separados. |
| E. Imagenes y arranque | `../tests/benchmarks/resultados/imagenes-stack-crudas.csv` y `../tests/benchmarks/resultados/arranque-stack-crudo.csv` | Cinco imagenes y un redeploy con volumen existente medidos; no es una mediana ni un arranque con volumen vacio. |
| F. Modificabilidad | Diff, tiempo y registro del algoritmo anadido en vivo | Se completa durante la demostracion. No crear implementacion previa ni rama preparada. |

## Guion de sustentacion

1. Presentar el problema, el alcance de CPU unica y los cuatro algoritmos implementados.
2. Recorrer la arquitectura Angular, dos microservicios, C/JNA, referencia Java, PostgreSQL y Keycloak.
3. Ejecutar una carga y comparar segmentos, metricas y promedios; mostrar la captura del Gantt de ocho procesos.
4. Mostrar autorizacion: estudiante consulta y recibe 403 al intentar escribir; JWT alterado se rechaza con 401.
5. Presentar los dos protocolos de benchmark y explicar por separado sus limites y resultados.
6. Realizar el ejercicio de modificabilidad solicitado por el docente en vivo. Registrar el estado inicial/final y los diffs de C, Java, Angular y OpenAPI; comprobar las restricciones de tiempo y modulos del requisito.
7. Cerrar con los limites de evidencia: sesiones RQ-03, control oficial y cualquier resultado no aceptado por el docente.

Este guion es una estructura de preparacion; no declara hecha la sustentacion ni la demostracion del algoritmo adicional.