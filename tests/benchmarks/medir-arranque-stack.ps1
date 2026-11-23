$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$results = Join-Path $PSScriptRoot 'resultados'
$deadline = [DateTime]::UtcNow.AddMinutes(10)

Push-Location $repo
try {
    $currentStacks = @(docker stack ls --format '{{.Name}}')
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo consultar Docker Swarm.' }
    if ($currentStacks -contains 'naufragos') { .\infra\swarm\detener.ps1 }
    do {
        $currentStacks = @(docker stack ls --format '{{.Name}}')
        if ($LASTEXITCODE -ne 0) { throw 'No se pudo consultar si el stack terminÃ³ de retirarse.' }
        if ($currentStacks -notcontains 'naufragos') { break }
        if ([DateTime]::UtcNow -ge $deadline) { throw 'El stack no terminÃ³ de retirarse dentro de 10 minutos.' }
        Start-Sleep -Seconds 2
    } while ($true)

    $startedAt = [DateTime]::UtcNow
    .\infra\swarm\iniciar.ps1
    if ($LASTEXITCODE -ne 0) { throw 'FallÃ³ el script de despliegue del stack.' }

    $urls = @(
        @{ url = 'http://localhost:4200'; expected = 200 },
        @{ url = 'http://localhost:8080/realms/planificador-realm/.well-known/openid-configuration'; expected = 200 },
        @{ url = 'http://localhost:8081/algoritmos'; expected = 401 },
        @{ url = 'http://localhost:8082/conjuntos'; expected = 401 }
    )
    do {
        $replicas = @(docker stack services naufragos --format '{{.Replicas}}')
        $allReplicasReady = $LASTEXITCODE -eq 0 -and $replicas.Count -eq 5 -and @($replicas | Where-Object { $_ -ne '1/1' }).Count -eq 0
        $allEndpointsReady = $true
        foreach ($endpoint in $urls) {
            try {
                $response = Invoke-WebRequest -Uri $endpoint.url -MaximumRedirection 0 -TimeoutSec 5 -UseBasicParsing
                $status = [int]$response.StatusCode
            } catch {
                $status = [int]$_.Exception.Response.StatusCode.value__
            }
            if ($status -ne $endpoint.expected) { $allEndpointsReady = $false }
        }
        if ($allReplicasReady -and $allEndpointsReady) { break }
        if ([DateTime]::UtcNow -ge $deadline) { throw 'Los cinco servicios y endpoints no estuvieron listos dentro de 10 minutos.' }
        Start-Sleep -Seconds 3
    } while ($true)

    $seconds = [Math]::Round(([DateTime]::UtcNow - $startedAt).TotalSeconds, 3)
    $row = [pscustomobject]@{
        fecha_utc = $startedAt.ToString('yyyy-MM-ddTHH:mm:ssZ')
        segundos_hasta_listo = $seconds
        servicios = 5
        replicas = '5/5'
        frontend_http = 200
        keycloak_oidc_http = 200
        planificador_sin_token_http = 401
        proceso_sin_token_http = 401
        volumen_postgres_vacio = $false
        imagenes_reconstruidas_sin_cache = $false
    }
    $row | Export-Csv -LiteralPath (Join-Path $results 'arranque-stack-crudo.csv') -NoTypeInformation -Encoding utf8
    Write-Host "Stack completo listo en $seconds s; cinco servicios 1/1, frontend/OIDC 200 y APIs sin token 401. Volumen existente conservado."
} finally {
    Pop-Location
}
