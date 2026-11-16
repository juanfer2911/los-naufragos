$ErrorActionPreference = 'Stop'
$results = Join-Path $PSScriptRoot 'resultados'
$stamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$stdoutLog = Join-Path $results "primer-resultado-$stamp.stdout.log"
$stderrLog = Join-Path $results "primer-resultado-$stamp.stderr.log"
$argsDocker = @(
    'run','--rm','--cpus=0.75','--memory=768m',
    '--env','BENCHMARK_CONTAINER_LIMITED=true',
    '--env','BENCHMARK_PROTOCOL=arquitectura',
    '--env','BENCHMARK_CSV=/tmp/arquitectura-primer-resultado-crudos.csv',
    'naufragos-benchmark:local'
)
$elapsed = Measure-Command {
    $process = Start-Process -FilePath 'docker.exe' -ArgumentList $argsDocker `
        -Wait -PassThru -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog
}
if ($process.ExitCode -ne 0) { throw "Falló la medición. Revisa $stdoutLog y $stderrLog." }
$firstResult = Select-String -Path $stdoutLog -Pattern 'Arquitectura: repeticion 1/5 completada'
if (-not $firstResult) { throw "No se encontró la primera salida. Revisa $stdoutLog." }
$seconds = [Math]::Round($elapsed.TotalSeconds, 3)
[pscustomobject]@{
    protocolo = 'arquitectura'
    procesos = 10000
    cpu_limit = '0.75'
    memory_limit_mib = 768
    container_wall_seconds = $seconds
    first_result_upper_bound_seconds = $seconds
    threshold_seconds = 90
    threshold_met = ($seconds -lt 90)
} | Export-Csv -LiteralPath (Join-Path $results 'primer-resultado-contenedor.csv') -NoTypeInformation -Encoding utf8
Write-Host "La primera salida apareció dentro de $seconds s como máximo (todo el contenedor terminó en ese tiempo)."
