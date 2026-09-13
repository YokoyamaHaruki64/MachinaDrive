param()

# System.Drawing.Commonの参照差異を避けるため、Windows PowerShellでだけ実行する。
if ($PSVersionTable.PSEdition -ne "Desktop")
{
    throw "Windows PowerShell (powershell.exe) で実行してください。"
}

Add-Type -AssemblyName System.Drawing

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$outputDirectory = Join-Path $repositoryRoot "Assets\Local\Textures\VFX"
$outputPath = Join-Path $outputDirectory "VFX_TimeBeam.png"

New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$width = 96
$height = 384
$bitmap = New-Object System.Drawing.Bitmap($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

try
{
    for ($y = 0; $y -lt $height; ++$y)
    {
        $verticalRate = [double]$y / [double]($height - 1)
        $verticalFade = [Math]::Pow([Math]::Sin($verticalRate * [Math]::PI), 0.65)

        for ($x = 0; $x -lt $width; ++$x)
        {
            $horizontalRate = (($x + 0.5) / $width - 0.5) * 2.0
            $horizontalFade = [Math]::Exp(-8.0 * $horizontalRate * $horizontalRate)
            $alpha = [Math]::Min(255, [Math]::Floor(255.0 * $verticalFade * $horizontalFade))

            $bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int]$alpha, 255, 255, 255))
        }
    }

    $bitmap.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally
{
    $bitmap.Dispose()
}

Write-Host "生成しました: $outputPath ($width x $height, 透明背景)"
