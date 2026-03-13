param (
    [Parameter(Mandatory=$true)] [int]$w,
    [Parameter(Mandatory=$true)] [int]$h,
    [string]$inputDir = ".",
    [string]$outputDir = ""
)

$inputDir = (Resolve-Path $inputDir).Path
if ([string]::IsNullOrWhiteSpace($outputDir)) {
    $outputDir = Join-Path $inputDir "resize_output"
}

if (!(Test-Path $outputDir)) { New-Item -ItemType Directory -Path $outputDir | Out-Null }

$files = Get-ChildItem "$inputDir\*.gif"

foreach ($file in $files) {
    Write-Host "Processing: $($file.Name)..." -ForegroundColor Cyan
    
    $size = ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=s=x:p=0 "$($file.FullName)"
    $dimensions = $size.Split('x')
    $origW = [int]$dimensions[0]
    $origH = [int]$dimensions[1]

    $outPath = Join-Path $outputDir $file.Name

    $filter = "[0:v]scale=${w}:${h}:force_original_aspect_ratio=decrease,setsar=1,pad=${w}:${h}:(ow-iw)/2:(oh-ih)/2:color=00000000,split[p1][p2];[p1]palettegen=max_colors=4:reserve_transparent=on[pal];[p2][pal]paletteuse=dither=none"
    ffmpeg -i "$($file.FullName)" -filter_complex $filter "$outPath" -y -loglevel error

    Write-Host "=> Done: $($file.Name) (Original: $($origW)x$($origH) -> Target: ${w}x${h})" -ForegroundColor Green
}

Write-Host "`nAll tasks completed!" -ForegroundColor Magenta
