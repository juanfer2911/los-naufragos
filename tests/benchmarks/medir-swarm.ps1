$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$results = Join-Path $PSScriptRoot 'resultados'
$image = 'naufragos-benchmark:local'
$imageId = $null
Push-Location $repo
try {
    docker build -t $image -f tests/benchmarks/Dockerfile .
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo construir la imagen aislada del banco.' }
    $imageId = docker image inspect --format '{{.Id}}' $image

    foreach ($protocol in @('web2', 'arquitectura')) {
        $container = "naufragos-benchmark-$protocol"
        $old = docker ps -aq --filter "name=^/$container$"
        if ($old) { docker rm -f $container | Out-Null }

        $rawName = "$protocol-swarm-crudos.csv"
        $rawPath = Join-Path $results $rawName
        $summaryPath = Join-Path $results "$protocol-swarm-crudos-medianas.csv"
        $logPath = Join-Path $results "$protocol-swarm.log"
        $runArgs = @(
            'run', '--name', $container, '--cpus=0.75', '--memory=768m',
            '--env', "BENCHMARK_PROTOCOL=$protocol",
            '--env', "BENCHMARK_CSV=/tmp/$rawName",
            '--env', 'BENCHMARK_CONTAINER_LIMITED=true',
            $image
        )
        & docker @runArgs *> $logPath
        if ($LASTEXITCODE -ne 0) { throw "Falló el banco $protocol. Revisa $logPath y los registros del contenedor $container." }

        docker cp "${container}:/tmp/$rawName" $rawPath
        if ($LASTEXITCODE -ne 0) { throw "No se pudo copiar el CSV crudo de $protocol." }
        docker cp "${container}:/tmp/$protocol-swarm-crudos-medianas.csv" $summaryPath
        if ($LASTEXITCODE -ne 0) { throw "No se pudo copiar el CSV de medianas de $protocol." }
        $limits = docker inspect --format '{{.HostConfig.NanoCpus}},{{.HostConfig.Memory}}' $container
        if ($limits -ne '750000000,805306368') { throw "Límites inesperados en el contenedor ${container}: $limits" }
        docker rm $container | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "No se pudo retirar el contenedor finalizado $container." }
        Write-Host "$protocol completado con límites verificados de 0.75 CPU y 768 MiB. Resultados: $rawPath"
    }

    @(
        'Perfil: mismos límites de CPU y memoria que los microservicios en infra/swarm/stack.yml.'
        'Límite CPU: 0.75 vCPU (Docker NanoCpus 750000000).'
        'Límite memoria: 768 MiB (Docker Memory 805306368 bytes).'
        "Imagen banco: $imageId"
        "Fecha UTC: $([DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ'))"
    ) | Set-Content -LiteralPath (Join-Path $results 'swarm-recursos.txt') -Encoding utf8
} finally {
    Pop-Location
}
