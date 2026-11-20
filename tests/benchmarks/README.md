# Bancos de mediciÃ³n C/JNA frente a Java

Los dos protocolos ejecutan directamente `PlanificadorNativo` y `PlanificadorJava`, sin Spring, HTTP, autenticaciÃ³n ni escritura en PostgreSQL. Cada muestra genera el resultado completo y exige igualdad exacta de segmentos y mÃ©tricas.

## EjecuciÃ³n local

Desde Ubuntu/WSL con Java 21, Maven y la biblioteca Linux:

```sh
make -C native library
bash tests/benchmarks/medir-web2.sh
bash tests/benchmarks/medir-arquitectura.sh
```

Los scripts aceptan la ruta de `libplanificador.so` y el CSV crudo de salida como argumentos opcionales. Los datos crudos y medianas predeterminados quedan en `resultados/`. No leen `.env` ni requieren bases de datos.

## Protocolos

- **Web 2:** FCFS, SJF, SRTF y RR; 50, 500, 5.000 y 50.000 procesos; 30 repeticiones por algoritmo y tamaÃ±o. Quantum RR: 10.
- **Arquitectura:** SRTF con 10.000 procesos; cinco ejecuciones, descarta las tres primeras y toma la mediana de las dos Ãºltimas.
- Semilla base `20261116`; llegada uniforme `[0, 2N]`, rÃ¡faga `[1, 100]`, prioridad `[1, 5]`, PID por orden de ingreso. El orden de los motores alterna.
- El cronÃ³metro rodea solo `ejecutar`; generaciÃ³n, verificaciÃ³n de igualdad y escritura quedan fuera.
- `c_jna` es el adaptador Java/JNA con el sondeo y la llamada de ejecuciÃ³n de la ABI actual; no es C sin JNA.
- En Web 2, el perfil limitado fuerza GC tras cada muestra con N >= 5.000 para liberar memoria temporal de JNA. La pausa queda fuera del tiempo medido; el perfil local no fuerza esas colecciones.

## EjecuciÃ³n dentro de contenedor limitado

Con Docker Swarm iniciado y PowerShell:

```powershell
.\tests\benchmarks\medir-swarm.ps1
```

Construye una imagen runner aislada y ejecuta ambos bancos secuencialmente con `--cpus=0.75` y `--memory=768m`, los lÃ­mites declarados para cada microservicio en `infra/swarm/stack.yml`. Comprueba los lÃ­mites efectivos y guarda CSV crudos, medianas y `swarm-recursos.txt` sin modificar resultados locales ni datos de las bases. Si un banco falla, conserva el registro y el contenedor para inspecciÃ³n.

## Resultados locales WSL, 2026-11-16

Ubuntu 26.04, Linux x86-64, Intel Core i9-13900H, Java 21.0.13, GCC 15.2. Sin lÃ­mites de CPU o memoria. Medianas en ms:

| Algoritmo | N | C/JNA | Java | N | C/JNA | Java |
|---|---:|---:|---:|---:|---:|---:|
| FCFS | 50 | 2,126 | 0,191 | 500 | 3,311 | 0,640 |
| FCFS | 5.000 | 30,738 | 4,274 | 50.000 | 353,030 | 58,056 |
| SJF | 50 | 0,817 | 0,213 | 500 | 2,414 | 0,603 |
| SJF | 5.000 | 38,853 | 6,190 | 50.000 | 447,733 | 101,676 |
| SRTF | 50 | 0,815 | 0,489 | 500 | 2,493 | 0,694 |
| SRTF | 5.000 | 23,381 | 5,065 | 50.000 | 417,113 | 85,566 |
| RR | 50 | 0,658 | 0,310 | 500 | 5,056 | 0,919 |
| RR | 5.000 | 76,160 | 3,990 | 50.000 | 1.290,957 | 48,352 |

Arquitectura, SRTF y 10.000 procesos: C/JNA **67,520 ms**, Java **19,939 ms**. Las 970 comparaciones coincidieron exactamente. Java fue mÃ¡s rÃ¡pido en los 16 pares Web 2; no hubo cruce entre 50 y 50.000 procesos.

## Resultados Swarm limitados, 2026-11-16

Windows 11, Docker Desktop 29.7.2 con motor Linux/WSL2, Temurin JDK 21.0.7. LÃ­mite verificado de **0,75 CPU y 768 MiB**. Las tareas Swarm permanecieron activas durante la mediciÃ³n. Medianas en ms:

| Algoritmo | N | C/JNA | Java | N | C/JNA | Java |
|---|---:|---:|---:|---:|---:|---:|
| FCFS | 50 | 2,4 | 0,3 | 500 | 3,1 | 0,7 |
| FCFS | 5.000 | 81,9 | 5,5 | 50.000 | 831,7 | 188,4 |
| SJF | 50 | 0,7 | 0,2 | 500 | 3,9 | 1,1 |
| SJF | 5.000 | 23,7 | 5,7 | 50.000 | 932,2 | 219,3 |
| SRTF | 50 | 0,4 | 0,2 | 500 | 4,1 | 1,0 |
| SRTF | 5.000 | 75,9 | 7,0 | 50.000 | 974,4 | 209,5 |
| RR | 50 | 0,8 | 0,2 | 500 | 5,2 | 0,9 |
| RR | 5.000 | 196,9 | 3,9 | 50.000 | 3.516,7 | 243,9 |

Arquitectura, SRTF y 10.000 procesos: C/JNA **299,336 ms**, Java **151,969 ms**. Las 970 comparaciones de ambos protocolos coincidieron exactamente; Java fue mÃ¡s rÃ¡pido en los 16 pares Web 2. Los resultados completos y los recursos verificados estÃ¡n en `resultados/*-swarm-*` y `resultados/swarm-recursos.txt`.

Estos valores caracterizan el runner limitado en Docker Desktop/WSL2 con el stack encendido. La JVM del runner es Temurin 21.0.7, frente a Java 21.0.13 en la mediciÃ³n local; considÃ©rese al comparar ambos entornos.


## Tamaño, construcción y primer resultado del planificador

La imagen runtime `naufragos-planificador` se construyó tres veces con `docker build --no-cache`. Los tiempos fueron **79,608 s**, **84,675 s** y **80,656 s**; mediana **80,656 s**. Los tamaños inspeccionados fueron 128.252.783, 128.252.846 y 128.252.783 bytes, todos por debajo del límite de 250.000.000 bytes. Una exportación Docker de la imagen midió 128.273.920 bytes. La comprobación `command -v gcc` dentro del contenedor runtime confirmó que no incluye GCC.

Para el umbral de primer resultado, el runner limitado inició el protocolo Arquitectura y produjo su primera salida SRTF con 10.000 procesos. El contenedor entero —inicio de JVM, cinco ejecuciones y escritura de resultados— terminó en **8,098 s**, por lo que el primer resultado apareció como máximo en ese tiempo. Esta medición directa del motor no incluye login, HTTP ni inicio en frío de Keycloak/PostgreSQL.

Evidencias crudas: `resultados/imagen-planificador-builds-crudos.csv`, `resultados/imagen-planificador-builds-mediana.csv` y `resultados/primer-resultado-contenedor.csv`; los registros de construcción y ejecución están junto a ellos.
Durante el despliegue inicial del stack en volumen nuevo, `planificador-service` pasó de inicio de contenedor `20:21:30.116Z` a aplicación lista `20:21:52.954Z` (**22,838 s**); Spring reportó 19,889 s de arranque interno. Es una observación de una sola ejecución, no una mediana ni una prueba de respuesta autenticada.
Tras preconstruir la configuración de Keycloak en su imagen, una actualización Swarm registró 56,778 s desde el inicio del contenedor hasta que Keycloak anunció que escuchaba en HTTP (55,919 s informados por Quarkus). La prueba fue sobre la base ya inicializada y el realm ya existente; no representa una inicialización nueva de PostgreSQL/realm ni una mediana del stack completo.
## Imágenes runtime y arranque del stack completo — 2026-11-20 UTC

Se inspeccionaron las cinco imágenes runtime usadas por el stack (`docker image inspect`). Los tamaños están en bytes y MB decimales; los datos crudos están en `resultados/imagenes-stack-crudas.csv`.

| Imagen | Bytes | MB decimales |
|---|---:|---:|
| Frontend | 23.184.765 | 23,185 |
| Keycloak | 236.736.629 | 236,737 |
| PostgreSQL | 153.464.509 | 153,465 |
| proceso-service | 126.240.694 | 126,241 |
| planificador-service | 128.253.937 | 128,254 |

Se retiró y desplegó de nuevo el stack con `tests/benchmarks/medir-arranque-stack.ps1`. Desde que comenzó `iniciar.ps1` hasta que los cinco servicios estuvieron `1/1` y Frontend/OIDC respondieron 200 y ambas APIs respondieron 401 sin token transcurrieron **97,718 s**. El CSV crudo está en `resultados/arranque-stack-crudo.csv`.

La medición conserva el volumen PostgreSQL existente y usa la caché normal de construcción. Es un redeploy local del stack detenido, no un arranque con base vacía ni una mediana. No se eliminaron volúmenes ni datos.