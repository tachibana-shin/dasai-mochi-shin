param (
    [string]$InPath = ".",
    [string]$OutPath = ""
)

$absoluteInPath = (Resolve-Path $InPath).Path
if ([string]::IsNullOrWhiteSpace($OutPath)) {
    $absoluteOutPath = Join-Path $absoluteInPath "onebit_output"
} else {
    $absoluteOutPath = $OutPath
}

if (!(Test-Path $absoluteOutPath)) { New-Item -ItemType Directory -Path $absoluteOutPath | Out-Null }

Get-ChildItem -Path "$absoluteInPath\*.gif" | ForEach-Object {
    $inputFileName = $_.FullName
    $outputFileName = Join-Path $absoluteOutPath $_.Name

    Write-Host "Convert to one bit from: $($_.Name) (Keep original size)..." -ForegroundColor Cyan

    ffmpeg -i "$inputFileName" -filter_complex "[0:v]format=rgba,lutrgb=r=255:g=255:b=255,split[p1][p2];[p1]palettegen=max_colors=3:reserve_transparent=on[pal];[p2][pal]paletteuse=dither=none" "$outputFileName" -y -loglevel error
}

Write-Host "Done! File has been converted to single-color 1-bit." -ForegroundColor Green
