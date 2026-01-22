using assembly System.Drawing
using namespace System.Drawing
using namespace System.Drawing.Drawing2D
using namespace System.Drawing.Imaging
using namespace System.IO

$iconPath = Join-Path $PSScriptRoot "..\src\app_icon.ico"
$pngPath = Join-Path $PSScriptRoot "..\src\app_icon.png"

# 图标配置
$sizes = @(256, 48, 32, 16)
$bitmaps = @{}

# 绘图函数
function Draw-IconBitmap {
    param([int]$size)

    $bmp = New-Object Bitmap($size, $size)
    $g = [Graphics]::FromImage($bmp)
    $g.SmoothingMode = [SmoothingMode]::AntiAlias
    $g.InterpolationMode = [InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [PixelOffsetMode]::HighQuality

    # 1. 绘制背景 (圆角矩形)
    $rect = New-Object Rectangle(0, 0, $size, $size)
    $padding = [int]($size * 0.05)
    # 强制转换确保运算正确
    $drawSize = $size - ($padding * 2)
    $drawRect = New-Object Rectangle($padding, $padding, $drawSize, $drawSize)
    
    # 圆角半径
    $radius = [int]($size * 0.2)
    $path = New-Object GraphicsPath
    $path.AddArc([int]$drawRect.X, [int]$drawRect.Y, $radius, $radius, 180, 90)
    $path.AddArc([int]($drawRect.Right - $radius), [int]$drawRect.Y, $radius, $radius, 270, 90)
    $path.AddArc([int]($drawRect.Right - $radius), [int]($drawRect.Bottom - $radius), $radius, $radius, 0, 90)
    $path.AddArc([int]$drawRect.X, [int]($drawRect.Bottom - $radius), $radius, $radius, 90, 90)
    $path.CloseFigure()

    # 填充背景 (纯白)
    $brushBg = New-Object SolidBrush([Color]::White)
    $g.FillPath($brushBg, $path)

    # 描边 (深灰)
    $borderWidth = [Math]::Max(1, [int]($size * 0.04))
    $penBorder = New-Object Pen([Color]::FromArgb(255, 60, 60, 60), [float]$borderWidth)
    $g.DrawPath($penBorder, $path)

    # 2. 绘制红色对勾 (P0)
    $checkWidth = [Math]::Max(1.5, [float]($size * 0.08))
    $penCheck = New-Object Pen([Color]::FromArgb(255, 200, 0, 0), [float]$checkWidth)
    $penCheck.StartCap = [LineCap]::Round
    $penCheck.EndCap = [LineCap]::Round

    # 坐标点 - 显式转换为 float
    $p1 = New-Object PointF([float]($size * 0.25), [float]($size * 0.45))
    $p2 = New-Object PointF([float]($size * 0.40), [float]($size * 0.60))
    $p3 = New-Object PointF([float]($size * 0.75), [float]($size * 0.25))
    
    # 分段绘制
    $g.DrawLine($penCheck, $p1, $p2)
    $g.DrawLine($penCheck, $p2, $p3)

    # 3. 绘制列表线条
    $penLine = New-Object Pen([Color]::FromArgb(255, 180, 180, 180), [float]$checkWidth)
    $penLine.StartCap = [LineCap]::Round
    $penLine.EndCap = [LineCap]::Round

    # 线条坐标
    $lineX1 = [float]($size * 0.35)
    $lineX2 = [float]($size * 0.25)
    $lineEnd = [float]($size * 0.75)
    
    $y2 = [float]($size * 0.55)
    $y3 = [float]($size * 0.75)

    $g.DrawLine($penLine, $lineX1, $y2, $lineEnd, $y2)
    $g.DrawLine($penLine, $lineX2, $y3, $lineEnd, $y3)

    $g.Dispose()
    return $bmp
}

# 生成所有尺寸的位图
try {
    foreach ($s in $sizes) {
        $bitmaps[$s] = Draw-IconBitmap -size $s
    }

    # 保存最大的为 PNG 预览
    if (Test-Path $pngPath) { Remove-Item $pngPath -Force }
    $bitmaps[256].Save($pngPath, [ImageFormat]::Png)
    Write-Host "Generated preview PNG at $pngPath"

    # 保存为 ICO
    if (Test-Path $iconPath) { Remove-Item $iconPath -Force }
    $fs = [File]::Open($iconPath, [FileMode]::Create)
    $bw = New-Object BinaryWriter($fs)

    # ICONDIR
    $bw.Write([int16]0) # Reserved
    $bw.Write([int16]1) # Type (1=Icon)
    $bw.Write([int16]$sizes.Count) # Count

    $offset = 6 + ($sizes.Count * 16)

    foreach ($s in $sizes) {
        $bmp = $bitmaps[$s]
        $ms = New-Object MemoryStream
        $bmp.Save($ms, [ImageFormat]::Png)
        $pngData = $ms.ToArray()
        $ms.Dispose()

        # ICONDIRENTRY
        $w = if ($s -eq 256) { 0 } else { [byte]$s }
        $bw.Write([byte]$w) # Width
        $bw.Write([byte]$w) # Height
        $bw.Write([byte]0)  # Colors
        $bw.Write([byte]0)  # Reserved
        $bw.Write([int16]1) # Planes
        $bw.Write([int16]32)# BitCount
        $bw.Write([int]$pngData.Length) # SizeInBytes
        $bw.Write([int]$offset) # ImageOffset

        $offset += $pngData.Length
        
        # 存储 PNG 数据
        $bmp.Tag = $pngData
    }

    # 写入图像数据
    foreach ($s in $sizes) {
        $pngData = $bitmaps[$s].Tag
        $bw.Write($pngData)
    }

    $bw.Close()
    $fs.Close()
    Write-Host "Generated ICO at $iconPath"
}
catch {
    Write-Error $_
}
finally {
    foreach ($key in $bitmaps.Keys) {
        if ($bitmaps[$key]) { $bitmaps[$key].Dispose() }
    }
}
