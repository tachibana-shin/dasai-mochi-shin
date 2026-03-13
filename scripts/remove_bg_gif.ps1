param (
    [string]$InPath = ".",
    [string]$OutPath = ""
)

$absoluteInPath = Resolve-Path $InPath

if ([string]::IsNullOrWhiteSpace($OutPath)) {
    $absoluteOutPath = Join-Path $absoluteInPath "output"
} else {
    $absoluteOutPath = $OutPath
}

if (!(Test-Path $absoluteOutPath)) { 
    New-Item -ItemType Directory -Path $absoluteOutPath | Out-Null 
}

Write-Host "Removing background from GIF files from: $absoluteInPath" -ForegroundColor Cyan
Write-Host "Results will be saved to: $absoluteOutPath" -ForegroundColor Cyan

Get-ChildItem -Path "$absoluteInPath\*.gif" | ForEach-Object {
    $inputFileName = $_.FullName
    $outputFileName = Join-Path $absoluteOutPath $_.Name
    
    Write-Host "Removing background from: $($_.Name)..." -ForegroundColor Yellow

    ffmpeg -i "$inputFileName" -vf "format=rgba,geq=r='r(X,Y)':g='g(X,Y)':b='b(X,Y)':a='if(lt(b(X,Y),150),0,255)'[base];[base]split[main][for_palette];[for_palette]geq=r='r(X,Y)':g='g(X,Y)':b='b(X,Y)':a='if(eq(X,0)*eq(Y,0),0,alpha(X,Y))'[forced];[forced]palettegen=reserve_transparent=on[p];[main][p]paletteuse" "$outputFileName" -y -loglevel error
}

Write-Host "Done!" -ForegroundColor Green
