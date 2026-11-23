$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$envFile = Join-Path $repo 'infra/.env'
if (-not (Test-Path $envFile)) {
    throw 'Falta infra/.env. Crea el archivo desde infra/.env.example y completa credenciales locales.'
}

$envValues = @{}
foreach ($line in Get-Content -LiteralPath $envFile) {
    if ($line -match '^\s*([A-Za-z_][A-Za-z0-9_]*)=(.*)$') {
        $envValues[$Matches[1]] = $Matches[2]
    }
}

$required = @('POSTGRES_ADMIN_USER','POSTGRES_ADMIN_PASSWORD','KEYCLOAK_DB_PASSWORD',
    'PROCESO_DB_PASSWORD','PLANIFICADOR_DB_PASSWORD','KEYCLOAK_ADMIN_USERNAME','KEYCLOAK_ADMIN_PASSWORD')
foreach ($name in $required) {
    $value = $envValues[$name]
    if ([string]::IsNullOrWhiteSpace($value) -or $value.StartsWith('change-this-')) {
        throw "Configura $name en infra/.env antes de desplegar."
    }
}

if (@(docker stack ls --format '{{.Name}}') -notcontains 'naufragos') {
    foreach ($port in @(4200,8080,8081,8082)) {
        $listener = Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue
        if ($listener) {
            throw "El puerto $port ya está ocupado. Cierra manualmente el servicio que lo usa y vuelve a ejecutar este script. No se detuvo ningún proceso."
        }
    }
}

Push-Location $repo
$previousEnv = @{}
try {
    foreach ($name in $envValues.Keys) {
        $previousEnv[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
        [Environment]::SetEnvironmentVariable($name, $envValues[$name], 'Process')
    }
    $previousEnv['PLANIFICADOR_MOTOR'] = [Environment]::GetEnvironmentVariable('PLANIFICADOR_MOTOR', 'Process')
    if (-not $envValues.ContainsKey('PLANIFICADOR_MOTOR')) {
        [Environment]::SetEnvironmentVariable('PLANIFICADOR_MOTOR', $null, 'Process')
    }
    docker info --format '{{.Swarm.LocalNodeState}}' | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Docker no está disponible. Inicia Docker Desktop y vuelve a intentarlo.' }
    $swarmState = docker info --format '{{.Swarm.LocalNodeState}}'
    if ($swarmState -eq 'inactive') { docker swarm init; if ($LASTEXITCODE -ne 0) { throw 'No se pudo iniciar Docker Swarm.' } }

    $secretMap = @{
        naufragos_postgres_admin_password = 'POSTGRES_ADMIN_PASSWORD'
        naufragos_keycloak_db_password = 'KEYCLOAK_DB_PASSWORD'
        naufragos_proceso_db_password = 'PROCESO_DB_PASSWORD'
        naufragos_planificador_db_password = 'PLANIFICADOR_DB_PASSWORD'
        naufragos_keycloak_admin_username = 'KEYCLOAK_ADMIN_USERNAME'
        naufragos_keycloak_admin_password = 'KEYCLOAK_ADMIN_PASSWORD'
    }
    $existingSecrets = @(docker secret ls --format '{{.Name}}')
    foreach ($secretName in $secretMap.Keys) {
        if ($existingSecrets -notcontains $secretName) {
            $secretPath = Join-Path ([IO.Path]::GetTempPath()) ([Guid]::NewGuid().ToString('N'))
            try {
                $secretValue = [Environment]::GetEnvironmentVariable($secretMap[$secretName], 'Process')
                [IO.File]::WriteAllText($secretPath, $secretValue, [Text.UTF8Encoding]::new($false))
                docker secret create $secretName $secretPath | Out-Null
                if ($LASTEXITCODE -ne 0) { throw "No se pudo crear el secreto Docker $secretName." }
            } finally {
                Remove-Item -LiteralPath $secretPath -Force -ErrorAction SilentlyContinue
            }
        }
    }

    docker build -t naufragos-postgres:local -f infra/swarm/postgres/Dockerfile .
    if ($LASTEXITCODE -ne 0) { throw 'Falló la imagen de PostgreSQL.' }
    docker build -t naufragos-keycloak:local -f infra/swarm/keycloak/Dockerfile .
    if ($LASTEXITCODE -ne 0) { throw 'Falló la imagen de Keycloak.' }
    docker build -t naufragos-proceso:local -f backend/proceso-service/Dockerfile .
    if ($LASTEXITCODE -ne 0) { throw 'Falló la imagen proceso-service.' }
    docker build -t naufragos-planificador:local -f backend/planificador-service/Dockerfile .
    if ($LASTEXITCODE -ne 0) { throw 'Falló la imagen planificador-service.' }
    docker build -t naufragos-frontend:local -f frontend/Dockerfile frontend
    if ($LASTEXITCODE -ne 0) { throw 'Falló la imagen frontend.' }

    docker stack deploy --compose-file infra/swarm/stack.yml naufragos
    if ($LASTEXITCODE -ne 0) { throw 'Falló el despliegue del stack.' }
    docker stack services naufragos
} finally {
    foreach ($name in $previousEnv.Keys) {
        [Environment]::SetEnvironmentVariable($name, $previousEnv[$name], 'Process')
    }
    Pop-Location
}
