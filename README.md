# Los Náufragos — Planificador de procesos PI

## Objetivo

Aplicación web didáctica para simular y comparar algoritmos de planificación de CPU sobre conjuntos de procesos. El sistema presentará diagramas de Gantt, intervalos de CPU ociosa y métricas de espera, retorno y respuesta. No ejecuta procesos reales del sistema operativo.

## Arquitectura prevista

- **Angular** en `frontend/`: interfaz para gestionar conjuntos, iniciar simulaciones y presentar resultados. No ejecuta los algoritmos ni calcula promedios.
- **`proceso-service`**, un microservicio Spring Boot: dueño de los procesos y conjuntos de prueba, con persistencia propia.
- **`planificador-service`**, un microservicio Spring Boot: responsable de las corridas, la comparación y la exposición de los resultados y algoritmos disponibles.
- **Núcleo nativo C** en `native/`: biblioteca compartida `libplanificador.so`, integrada desde `planificador-service` mediante JNA. Se prevé también una implementación Java de referencia seleccionable por configuración.
- **Keycloak**: proveedor de identidad OAuth 2.0/OIDC; los servicios aplicarán la autorización en el backend.

El diseño específico del equipo mantiene estos dos microservicios. La diferencia con el mínimo general de tres indicado en el enunciado está registrada como pendiente de aclaración; este README no incorpora un tercer servicio.

## Tecnologías previstas

| Estado | Tecnologías y decisiones |
|---|---|
| Confirmadas en los documentos | Angular con componentes standalone y signals; Spring Boot; C como biblioteca compartida Linux; JNA; Keycloak con OAuth 2.0/OIDC; Docker. |
| Pendientes de decisión | Kubernetes o Docker Swarm; versiones de Angular, Java, Spring Boot, JNA y Keycloak; motor de base de datos; contratos REST y C/JNA; persistencia del historial; alcance del algoritmo de prioridad. |

## Estructura principal

```text
.
├── backend/
│   ├── proceso-service/
│   └── planificador-service/
├── docs/
│   └── wireframes/
├── frontend/
├── infra/
├── native/
├── tests/
└── README.md
```

La estructura refleja la arquitectura prevista; la presencia de una carpeta no implica que la aplicación o el componente correspondiente ya estén implementados.

## Estado

El proyecto está en preparación: se han revisado los requisitos y se están preparando la arquitectura y la documentación del repositorio. **La implementación funcional todavía no ha comenzado.** No hay todavía proyectos Angular o Spring Boot, algoritmos nativos ni contratos técnicos definitivos.

## Documentación del proyecto

- [Enunciado general de Programación en Ambiente Web 2](docs/Enunciado%20general_PI.pdf)
- [Documento del equipo: Planificador de procesos](docs/-%20Los%20Naufragos%20-%20Planificador%20de%20procesos.pdf)
- [Documento del núcleo de planificación](docs/Los%20Naufragos%20-%20Nucleo%20de%20planificacion.pdf)
- [Guía maestra del proyecto](docs/Guia_maestra_Copilot_Planificador_Los_Naufragos.docx)
- [Borrador de arquitectura ADD](docs/arquitectura-add.md)
- [Borrador de bitácora de decisiones](docs/bitacora-decisiones.md)
- Wireframes en [`docs/wireframes/`](docs/wireframes/)