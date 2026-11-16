$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$results = Join-Path $PSScriptRoot 'resultados'
$stamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$rows = [System.Collections.Generic.List[object]]::new()
Push-Location $repo
try {
    foreach ($run in 1..3) {
        $tag = "naufragos-planificador:rq03-$stamp-$run"
        $stdoutLog = Join-Path $results "imagen-planificador-build-$stamp-$run.stdout.log"
        $stderrLog = Join-Path $results "imagen-planificador-build-$stamp-$run.stderr.log"
        $elapsed = Measure-Command {
            $process = Start-Process -FilePath 'docker.exe' `
                -ArgumentList @('build','--no-cache','--progress','plain','-t',$tag,'-f','backend/planificador-service/Dockerfile','.') `
                -WorkingDirectory $repo -Wait -PassThru `
                -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog
        }
        if ($process.ExitCode -ne 0) { throw "Falló la construcción $run. Revisa $stdoutLog y $stderrLog." }

        $bytes = [long](docker image inspect --format '{{.Size}}' $tag)
        if ($LASTEXITCODE -ne 0) { throw "No se pudo inspeccionar la imagen $run." }
        $probe = docker run --rm --entrypoint /bin/sh $tag -c `
            'if command -v gcc >/dev/null 2>&1; then echo presente; else echo ausente; fi'
        if ($LASTEXITCODE -ne 0 -or $probe -ne 'ausente') {
            throw "La imagen $run contiene GCC o no se pudo comprobar."
        }
        $rows.Add([pscustomobject]@{
            build = $run
            tag = $tag
            segundos = [Math]::Round($elapsed.TotalSeconds, 3)
            image_bytes = $bytes
            gcc_runtime = $false
        })
        Write-Host "Construcción $run/3: $([Math]::Round($elapsed.TotalSeconds, 3)) s, $bytes bytes, GCC ausente."
    }

    $rawPath = Join-Path $results 'imagen-planificador-builds-crudos.csv'
    $rows | Export-Csv -LiteralPath $rawPath -NoTypeInformation -Encoding utf8
    $sorted = @($rows | ForEach-Object { $_.segundos } | Sort-Object)
    $median = $sorted[1]
    $maxImageBytes = [long](($rows | Measure-Object -Property image_bytes -Maximum).Maximum)
    $summary = [pscustomobject]@{
        construcciones = 3
        mediana_segundos = $median
        image_bytes = $rows[0].image_bytes
        max_image_bytes = $maxImageBytes
        limite_requisito_bytes = 250000000
        dentro_del_limite = ($maxImageBytes -lt 250000000)
        gcc_runtime = $false
    }
    $summary | Export-Csv -LiteralPath (Join-Path $results 'imagen-planificador-builds-mediana.csv') -NoTypeInformation -Encoding utf8
    Write-Host "Mediana de construcción: $median s. CSV: $rawPath"
} finally {
    Pop-Location
}
