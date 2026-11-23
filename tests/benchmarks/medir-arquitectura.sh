#!/usr/bin/env bash
set -euo pipefail

RAIZ="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
BIBLIOTECA="${1:-$RAIZ/native/libplanificador.so}"
SALIDA="${2:-$RAIZ/tests/benchmarks/resultados/arquitectura-crudos.csv}"

cd "$RAIZ"
POM="$RAIZ/backend/planificador-service/pom.xml"
mvn -f "$POM" -DskipTests compile
CP_FILE="$(mktemp)"
trap 'rm -f "$CP_FILE"' EXIT
mvn -q -f "$POM" "-Dmdep.outputFile=$CP_FILE" dependency:build-classpath
CLASSPATH="$RAIZ/backend/planificador-service/target/classes:$(cat "$CP_FILE")"
BENCHMARK_PROTOCOL=arquitectura PLANIFICADOR_BIBLIOTECA="$BIBLIOTECA" BENCHMARK_CSV="$SALIDA" \
  java -cp "$CLASSPATH" com.losnaufragos.planificador.nativo.BenchmarkMotores
