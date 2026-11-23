$ErrorActionPreference = 'Stop'
docker stack rm naufragos
if ($LASTEXITCODE -ne 0) { throw 'No se pudo retirar el stack naufragos.' }
Write-Host 'Stack retirado. El volumen naufragos_postgres_data se conserva.'
