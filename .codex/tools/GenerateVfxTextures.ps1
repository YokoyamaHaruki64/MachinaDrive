param(
    [string]$OutputDirectory = "Assets/Local/Textures/VFX",
    [ValidateRange(32, 2048)]
    [int]$Size = 128
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($PSVersionTable.PSEdition -ne "Desktop")
{
    throw "Run this script with Windows PowerShell (powershell.exe)."
}

Add-Type -AssemblyName System.Drawing

function Get-Gaussian([float]$Value, [float]$Sharpness)
{
    return [float][System.Math]::Exp(-$Value * $Value * $Sharpness)
}

function Get-SoftBand([float]$Distance, [float]$HalfWidth, [float]$Softness)
{
    return [System.Math]::Max(0.0, [System.Math]::Min(1.0, ($HalfWidth + $Softness - $Distance) / $Softness))
}

function Save-SparkTexture([string]$Path, [int]$TextureSize)
{
    $bitmap = New-Object System.Drawing.Bitmap($TextureSize, $TextureSize, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try
    {
        for ($y = 0; $y -lt $TextureSize; ++$y)
        {
            for ($x = 0; $x -lt $TextureSize; ++$x)
            {
                $u = (($x + 0.5) / $TextureSize) * 2.0 - 1.0
                $v = (($y + 0.5) / $TextureSize) * 2.0 - 1.0
                $radius = [System.Math]::Sqrt($u * $u + $v * $v)

                $core = Get-Gaussian $radius 42.0
                $horizontal = (Get-Gaussian $v 520.0) * (Get-Gaussian $u 2.8)
                $vertical = (Get-Gaussian $u 520.0) * (Get-Gaussian $v 2.8)
                $diagonalA = (Get-Gaussian (($u - $v) * 0.70710678) 700.0) * (Get-Gaussian (($u + $v) * 0.70710678) 4.0)
                $diagonalB = (Get-Gaussian (($u + $v) * 0.70710678) 700.0) * (Get-Gaussian (($u - $v) * 0.70710678) 4.0)
                $rays = [System.Math]::Max([System.Math]::Max($horizontal, $vertical), [System.Math]::Max($diagonalA, $diagonalB))
                $edgeFade = Get-SoftBand $radius 1.08 0.28
                $alpha = [System.Math]::Max($core, $rays * 0.72) * $edgeFade
                $alpha = [System.Math]::Max(0.0, [System.Math]::Min(1.0, $alpha))

                $bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int][System.Math]::Floor($alpha * 255.0), 255, 255, 255))
            }
        }

        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally
    {
        $bitmap.Dispose()
    }
}

function Save-FlameTexture([string]$Path, [int]$TextureSize)
{
    $bitmap = New-Object System.Drawing.Bitmap($TextureSize, $TextureSize, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try
    {
        for ($y = 0; $y -lt $TextureSize; ++$y)
        {
            for ($x = 0; $x -lt $TextureSize; ++$x)
            {
                $u = (($x + 0.5) / $TextureSize) * 2.0 - 1.0
                $v = (($y + 0.5) / $TextureSize) * 2.0 - 1.0

                # 下端を広く、上端を細くすることで、粒子単体にも炎のシルエットを持たせる。
                $bottomFade = [System.Math]::Max(0.0, [System.Math]::Min(1.0, ($v + 1.0) / 0.18))
                $topFade = [System.Math]::Max(0.0, [System.Math]::Min(1.0, (0.96 - $v) / 0.38))
                $verticalMask = $bottomFade * $topFade
                $height = ($v + 1.0) * 0.5
                $width = 0.38 * (1.0 - $height) + 0.045
                $center = 0.08 * [System.Math]::Sin($v * 5.5)

                $body = Get-Gaussian (($u - $center) / $width) 2.8
                $leftLobe = Get-Gaussian (($u + 0.18 + $center * 0.5) / ($width * 0.72)) 4.0
                $rightLobe = Get-Gaussian (($u - 0.20 + $center * 0.4) / ($width * 0.62)) 4.5
                $tip = Get-Gaussian (($u - $center * 1.8) / 0.055) 2.2 * [System.Math]::Max(0.0, [System.Math]::Min(1.0, ($v - 0.45) / 0.45))
                $alpha = [System.Math]::Max($body * 0.86, [System.Math]::Max($leftLobe, $rightLobe) * 0.38)
                $alpha = [System.Math]::Max($alpha, $tip)
                $alpha *= $verticalMask
                $alpha = [System.Math]::Max(0.0, [System.Math]::Min(1.0, $alpha))

                $bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int][System.Math]::Floor($alpha * 255.0), 255, 255, 255))
            }
        }

        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally
    {
        $bitmap.Dispose()
    }
}

function Save-RecoveryPlusTexture([string]$Path, [int]$TextureSize)
{
    $bitmap = New-Object System.Drawing.Bitmap($TextureSize, $TextureSize, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try
    {
        for ($y = 0; $y -lt $TextureSize; ++$y)
        {
            for ($x = 0; $x -lt $TextureSize; ++$x)
            {
                $u = (($x + 0.5) / $TextureSize) * 2.0 - 1.0
                $v = (($y + 0.5) / $TextureSize) * 2.0 - 1.0
                $vertical = (Get-SoftBand ([System.Math]::Abs($u)) 0.105 0.028) * (Get-SoftBand ([System.Math]::Abs($v)) 0.46 0.035)
                $horizontal = (Get-SoftBand ([System.Math]::Abs($v)) 0.105 0.028) * (Get-SoftBand ([System.Math]::Abs($u)) 0.46 0.035)
                $alpha = [System.Math]::Max($vertical, $horizontal)
                $alpha = [System.Math]::Max(0.0, [System.Math]::Min(1.0, $alpha))

                $bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int][System.Math]::Floor($alpha * 255.0), 255, 255, 255))
            }
        }

        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally
    {
        $bitmap.Dispose()
    }
}

function Save-GrayscaleTexture([string]$SourcePath, [string]$Path)
{
    if (-not (Test-Path -LiteralPath $SourcePath))
    {
        throw "Source texture was not found: $SourcePath"
    }

    $source = New-Object System.Drawing.Bitmap($SourcePath)
    $bitmap = New-Object System.Drawing.Bitmap($source.Width, $source.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try
    {
        for ($y = 0; $y -lt $source.Height; ++$y)
        {
            for ($x = 0; $x -lt $source.Width; ++$x)
            {
                $pixel = $source.GetPixel($x, $y)
                $luma = [int][System.Math]::Floor($pixel.R * 0.2126 + $pixel.G * 0.7152 + $pixel.B * 0.0722)
                $luma = [System.Math]::Max(0, [System.Math]::Min(255, $luma))
                $bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($pixel.A, $luma, $luma, $luma))
            }
        }

        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally
    {
        $source.Dispose()
        $bitmap.Dispose()
    }
}

New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
Save-SparkTexture (Join-Path $OutputDirectory "VFX_Spark.png") $Size
Save-FlameTexture (Join-Path $OutputDirectory "VFX_Flame.png") $Size
Save-RecoveryPlusTexture (Join-Path $OutputDirectory "VFX_RecoveryPlus.png") $Size
Save-GrayscaleTexture "Assets/Local/Textures/trail.png" (Join-Path $OutputDirectory "VFX_AttackChargeTrail.png")
Write-Host "Generated VFX textures: $OutputDirectory"
