param([string]$Compiler = 'cl', [switch]$RenderSmoke)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Push-Location $repo
try {
    $output = Join-Path $repo 'Build/Tests'
    New-Item -ItemType Directory -Force $output | Out-Null
    $sources = @('Tests/ControlsTests.cpp', 'FHGUI/Elements/Controls.cpp',
        'FHGUI/Elements/Inputs.cpp', 'FHGUI/Elements/Window.cpp', 'FHGUI/Input.cpp')
    $exe = Join-Path $output 'ControlsTests.exe'
    if ($RenderSmoke) {
        $sources = @('Tests/RenderSmoke.cpp', 'DirectX/DirectX.cpp',
            'FHGUI/Elements/Controls.cpp', 'FHGUI/Elements/Inputs.cpp',
            'FHGUI/Elements/Window.cpp', 'FHGUI/FHGUI.cpp', 'FHGUI/Input.cpp',
            'Menu/Menu.cpp', 'Render/D3DFont.cpp', 'Render/Render.cpp')
        $exe = Join-Path $output 'RenderSmoke.exe'
    }
    if ([IO.Path]::GetFileNameWithoutExtension($Compiler) -eq 'zig') {
        & $Compiler c++ -std=c++20 -fms-extensions -DUNICODE -D_UNICODE @sources -luser32 -lgdi32 -ld3d9 -o $exe
    } else {
        & $Compiler /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE @sources "/Fe:$exe" "/Fo:$output/" /link user32.lib gdi32.lib d3d9.lib
    }
    if ($LASTEXITCODE -ne 0) { throw 'Control tests failed to compile.' }
    if ($RenderSmoke) { & $exe (Join-Path $output 'showcase.bmp') }
    else { & $exe }
    if ($LASTEXITCODE -ne 0) { throw 'Control tests failed.' }
} finally { Pop-Location }
