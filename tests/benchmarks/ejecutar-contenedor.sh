#!/bin/sh
set -eu
classpath="$(cat /workspace/dependencies.classpath)"
exec java -cp "/workspace/target/classes:${classpath}" \
  com.losnaufragos.planificador.nativo.BenchmarkMotores
