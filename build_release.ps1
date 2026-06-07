# FloatingUniverse Release Build Script
param(
    [ValidateSet("all", "portable", "singleexe", "installer")]
    [string]$Mode = "all",
    [string]$SignCertPath = "",
    [string]$SignPassword = "",
    [string]$QtPath = "D:\Software\Qt\6.11.1\mingw_64\bin",
    [string]$MingwPath = "D:\Software\Qt\Tools\mingw1310_64\bin",
    [string]$InnoSetupPath = "",
    [string]$SevenZipPath = "C:\Program Files\7-Zip\7z.exe"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
$AppVersion = "1.0.2"
$AppName = "FloatingUniverse"
$BuildDir = Join-Path $ProjectRoot "build"
$ReleaseDir = Join-Path $ProjectRoot "release"

$env:PATH = "$QtPath;$MingwPath;$env:PATH"

function Log { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function LogOk { param($msg) Write-Host "[OK] $msg" -ForegroundColor Green }
function LogErr { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }
function LogWarn { param($msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }

function CheckDeps {
    Log "Checking Qt environment..."
    if (-not (Test-Path (Join-Path $QtPath "qmake.exe"))) {
        LogErr "qmake.exe not found at $QtPath"
        exit 1
    }
    if (-not (Test-Path (Join-Path $MingwPath "mingw32-make.exe"))) {
        LogErr "mingw32-make.exe not found at $MingwPath"
        exit 1
    }
    if (-not (Test-Path (Join-Path $QtPath "windeployqt.exe"))) {
        LogErr "windeployqt.exe not found"
        exit 1
    }
    if (-not (Test-Path $SevenZipPath)) {
        LogWarn "7z.exe not found, single exe packaging may fail"
    }

    if ($InnoSetupPath -eq "") {
        $candidates = @(
            "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
            "C:\Program Files\Inno Setup 6\ISCC.exe",
            "C:\Program Files (x86)\Inno Setup 5\ISCC.exe",
            "C:\Program Files\Inno Setup 5\ISCC.exe"
        )
        foreach ($c in $candidates) {
            if (Test-Path $c) { $InnoSetupPath = $c; break }
        }
    }

    if ($InnoSetupPath -eq "" -or -not (Test-Path $InnoSetupPath)) {
        LogWarn "Inno Setup not found, installer packaging unavailable"
        $script:InnoSetupPath = ""
    } else {
        LogOk "Found Inno Setup: $InnoSetupPath"
    }
}

function BuildProject {
    Log "Building project..."
    # Clean build files (ignore errors)
    if (Test-Path (Join-Path $ProjectRoot "Makefile.Release")) {
        Remove-Item -Path (Join-Path $ReleaseDir "*") -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $ProjectRoot "Makefile*") -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $ProjectRoot "*.o") -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $ProjectRoot "*.a") -Force -ErrorAction SilentlyContinue
        Get-ChildItem $ProjectRoot -Filter "*.h" -Name | Where-Object { $_ -like "ui_*" } | ForEach-Object { Remove-Item (Join-Path $ProjectRoot $_) -Force -ErrorAction SilentlyContinue }
        Get-ChildItem $ProjectRoot -Filter "*.cpp" -Name | Where-Object { $_ -like "moc_*" -or $_ -like "qrc_*" } | ForEach-Object { Remove-Item (Join-Path $ProjectRoot $_) -Force -ErrorAction SilentlyContinue }
    }

    Log "Running qmake..."
    Push-Location $ProjectRoot
    & qmake FloatingUniverse.pro -spec win32-g++ "CONFIG+=release"
    if ($LASTEXITCODE -ne 0) { LogErr "qmake failed"; exit 1 }
    LogOk "qmake done"

    Log "Compiling..."
    & mingw32-make -j8
    if ($LASTEXITCODE -ne 0) { LogErr "Compilation failed"; exit 1 }
    LogOk "Compilation done"
    Pop-Location

    Log "Deploying Qt dependencies..."
    $exePath = Join-Path $ReleaseDir "$AppName.exe"
    if (-not (Test-Path $exePath)) { LogErr "Executable not found: $exePath"; exit 1 }

    & windeployqt --release $exePath
    if ($LASTEXITCODE -ne 0) { LogErr "windeployqt failed"; exit 1 }
    LogOk "Qt dependencies deployed"
}

function SignFile {
    param($filePath)
    if ($SignCertPath -eq "" -or -not (Test-Path $SignCertPath)) {
        LogWarn "No signing cert provided, skipping: $filePath"
        return
    }
    Log "Signing: $filePath"

    $signTool = "signtool.exe"
    if (-not (Get-Command $signTool -ErrorAction SilentlyContinue)) {
        $sdkPaths = @(
            "C:\Program Files (x86)\Windows Kits\10\bin\x64\signtool.exe",
            "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
        )
        foreach ($p in $sdkPaths) {
            if (Test-Path $p) { $signTool = $p; break }
        }
    }

    $signArgs = @("sign", "/f", $SignCertPath, "/fd", "SHA256", "/t", "http://timestamp.digicert.com")
    if ($SignPassword -ne "") {
        $signArgs += @("/p", $SignPassword)
    }
    $signArgs += $filePath

    & $signTool $signArgs
    if ($LASTEXITCODE -eq 0) { LogOk "Signed: $filePath" }
    else { LogErr "Sign failed: $filePath" }
}

function CreatePortable {
    Log "Creating Portable version..."
    $portableDir = Join-Path $BuildDir "Portable"
    if (Test-Path $portableDir) { Remove-Item $portableDir -Recurse -Force }
    New-Item -ItemType Directory -Path $portableDir -Force | Out-Null

    Copy-Item -Path (Join-Path $ReleaseDir "*") -Destination $portableDir -Recurse -Force
    if (Test-Path (Join-Path $ProjectRoot "dist\data")) {
        Copy-Item -Path (Join-Path $ProjectRoot "dist\data") -Destination $portableDir -Recurse -Force
    }

    SignFile (Join-Path $portableDir "$AppName.exe")

    $zipPath = Join-Path $BuildDir "${AppName}_v${AppVersion}_Portable.zip"
    Log "Creating zip: $zipPath"
    Compress-Archive -Path (Join-Path $portableDir "*") -DestinationPath $zipPath -Force
    LogOk "Portable version created: $zipPath"
}

function CreateSingleExe {
    Log "Creating Single EXE version..."
    $singleDir = Join-Path $BuildDir "SingleExe"
    if (Test-Path $singleDir) { Remove-Item $singleDir -Recurse -Force }
    New-Item -ItemType Directory -Path $singleDir -Force | Out-Null

    Copy-Item -Path (Join-Path $ReleaseDir "*") -Destination $singleDir -Recurse -Force
    if (Test-Path (Join-Path $ProjectRoot "dist\data")) {
        Copy-Item -Path (Join-Path $ProjectRoot "dist\data") -Destination $singleDir -Recurse -Force
    }

    SignFile (Join-Path $singleDir "$AppName.exe")

    if (-not (Test-Path $SevenZipPath)) {
        LogErr "7z.exe not found, cannot create self-extracting archive"
        return
    }

    $sfxConfigPath = Join-Path $singleDir "sfx_config.txt"
    $sfxLines = @()
    $sfxLines += ";!@Install@!UTF-8!"
    $sfxLines += "Title=`"$AppName v$AppVersion`""
    $sfxLines += "BeginPrompt=`"$AppName v$AppVersion Extract and Run?`""
    $sfxLines += "ExecuteFile=`"$AppName.exe`""
    $sfxLines += "Directory=`".\$AppName`""
    $sfxLines += "GUIMode=`"2`""
    $sfxLines += ";!@InstallEnd@!"
    Set-Content -Path $sfxConfigPath -Value $sfxLines -Encoding UTF8

    $sfxModule = Join-Path (Split-Path $SevenZipPath) "7z.sfx"
    if (-not (Test-Path $sfxModule)) {
        LogErr "7z.sfx module not found"
        return
    }

    $outputExe = Join-Path $BuildDir "${AppName}_v${AppVersion}_Single.exe"
    $temp7z = Join-Path $singleDir "temp.7z"

    Push-Location $singleDir
    & $SevenZipPath a -t7z $temp7z ".\*" -mx=9
    Pop-Location

    $cmdLine = 'copy /b "{0}"+"{1}"+"{2}" "{3}"' -f $sfxModule, $sfxConfigPath, $temp7z, $outputExe
    cmd /c $cmdLine

    Remove-Item $temp7z -Force
    Remove-Item $sfxConfigPath -Force

    if (Test-Path $outputExe) {
        SignFile $outputExe
        LogOk "Single EXE version created: $outputExe"
    } else {
        LogErr "Single EXE creation failed"
    }
}

function CreateInstaller {
    Log "Creating Installer version..."
    $installerDir = Join-Path $BuildDir "InstallerSource"
    if (Test-Path $installerDir) { Remove-Item $installerDir -Recurse -Force }
    New-Item -ItemType Directory -Path $installerDir -Force | Out-Null

    Copy-Item -Path (Join-Path $ReleaseDir "*") -Destination $installerDir -Recurse -Force
    if (Test-Path (Join-Path $ProjectRoot "dist\data")) {
        Copy-Item -Path (Join-Path $ProjectRoot "dist\data") -Destination $installerDir -Recurse -Force
    }

    SignFile (Join-Path $installerDir "$AppName.exe")

    if ($InnoSetupPath -eq "" -or -not (Test-Path $InnoSetupPath)) {
        LogErr "Inno Setup not found, cannot create installer"
        return
    }

    $issPath = Join-Path $BuildDir "installer.iss"
    $signLine = ""
    if ($SignCertPath -ne "" -and (Test-Path $SignCertPath)) {
        if ($SignPassword -ne "") {
            $signLine = "SignTool=signtool sign /f `"$SignCertPath`" /p `"$SignPassword`" /fd SHA256 /t `"http://timestamp.digicert.com`" `$f"
        } else {
            $signLine = "SignTool=signtool sign /f `"$SignCertPath`" /fd SHA256 /t `"http://timestamp.digicert.com`" `$f"
        }
    }

    $chineseIsl = Join-Path $ProjectRoot "dist\ChineseSimplified.isl"
    $setupIcon = Join-Path $ProjectRoot "SignIn.ico"

    $issLines = @()
    $issLines += "; FloatingUniverse Installer Script"
    $issLines += ""
    $issLines += "#define MyAppName `"$AppName`""
    $issLines += "#define MyAppVersion `"$AppVersion`""
    $issLines += "#define MyAppPublisher `"Hangzhou LanYixi Intelligent Technology Co., Ltd.`""
    $issLines += "#define MyAppURL `"https://github.com/xdsT2/FloatingUniverse_VS`""
    $issLines += "#define MyAppExeName `"$AppName.exe`""
    $issLines += "#define SourceDir `"$installerDir`""
    $issLines += ""
    $issLines += "[Setup]"
    $issLines += "AppId={{A8F3B7E2-4C5D-6E9F-1A2B-3C4D5E6F7A8B}"
    $issLines += "AppName={#MyAppName}"
    $issLines += "AppVersion={#MyAppVersion}"
    $issLines += "AppVerName={#MyAppName} {#MyAppVersion}"
    $issLines += "AppPublisher={#MyAppPublisher}"
    $issLines += "AppPublisherURL={#MyAppURL}"
    $issLines += "AppSupportURL={#MyAppURL}"
    $issLines += "AppUpdatesURL={#MyAppURL}"
    $issLines += "DefaultDirName={autopf}\{#MyAppName}"
    $issLines += "DefaultGroupName={#MyAppName}"
    $issLines += "AllowNoIcons=yes"
    $issLines += "OutputDir=$BuildDir"
    $issLines += "OutputBaseFilename=${AppName}_v${AppVersion}_Setup"
    $issLines += "SetupIconFile=$setupIcon"
    $issLines += "WizardStyle=modern"
    $issLines += "Compression=lzma2"
    $issLines += "SolidCompression=yes"
    $issLines += "ArchitecturesAllowed=x64"
    $issLines += "ArchitecturesInstallIn64BitMode=x64"
    $issLines += "PrivilegesRequired=lowest"
    $issLines += "ChangesEnvironment=no"
    $issLines += "DirExistsWarning=no"
    if ($signLine -ne "") { $issLines += $signLine }
    $issLines += ""
    $issLines += "[Languages]"
    $issLines += "Name: `"schinese`"; MessagesFile: `"$chineseIsl`""
    $issLines += ""
    $issLines += "[Tasks]"
    $issLines += "Name: `"desktopicon`"; Description: `"Create Desktop Shortcut`"; GroupDescription: `"Additional Icons:`"; Flags: unchecked"
    $issLines += ""
    $issLines += "[Files]"
    $issLines += "Source: `"{#SourceDir}\*`"; DestDir: `"{app}`"; Flags: ignoreversion recursesubdirs createallsubdirs"
    $issLines += ""
    $issLines += "[Icons]"
    $issLines += "Name: `"{group}\{#MyAppName}`"; Filename: `"{app}\{#MyAppExeName}`""
    $issLines += "Name: `"{group}\Uninstall {#MyAppName}`"; Filename: `"{uninstallexe}`""
    $issLines += "Name: `"{autodesktop}\{#MyAppName}`"; Filename: `"{app}\{#MyAppExeName}`"; Tasks: desktopicon"
    $issLines += ""
    $issLines += "[Run]"
    $issLines += "Filename: `"{app}\{#MyAppExeName}`"; Description: `"Run {#MyAppName}`"; Flags: nowait postinstall skipifsilent"
    $issLines += ""
    $issLines += "[UninstallDelete]"
    $issLines += "Type: filesandordirs; Name: `"{app}\data`""

    Set-Content -Path $issPath -Value $issLines -Encoding UTF8

    Log "Compiling installer with Inno Setup..."
    & $InnoSetupPath $issPath

    if ($LASTEXITCODE -eq 0) {
        $outputFile = Join-Path $BuildDir "${AppName}_v${AppVersion}_Setup.exe"
        if (Test-Path $outputFile) {
            SignFile $outputFile
            LogOk "Installer created: $outputFile"
        } else {
            LogErr "Installer compilation failed"
        }
    } else {
        LogErr "Inno Setup compilation failed"
    }
}

# Main
Log "========================================"
Log "  FloatingUniverse Release Builder"
Log "  Version: $AppVersion"
Log "  Mode: $Mode"
Log "========================================"

CheckDeps

switch ($Mode) {
    "all" {
        BuildProject
        CreatePortable
        CreateSingleExe
        CreateInstaller
    }
    "portable" {
        if (-not (Test-Path (Join-Path $ReleaseDir "$AppName.exe"))) { BuildProject }
        CreatePortable
    }
    "singleexe" {
        if (-not (Test-Path (Join-Path $ReleaseDir "$AppName.exe"))) { BuildProject }
        CreateSingleExe
    }
    "installer" {
        if (-not (Test-Path (Join-Path $ReleaseDir "$AppName.exe"))) { BuildProject }
        CreateInstaller
    }
}

Log "========================================"
Log "  Release Build Complete!"
Log "  Output Directory: $BuildDir"
Log "========================================"

Log "Generated files:"
Get-ChildItem $BuildDir -Recurse | Where-Object { $_.Extension -match "\.(exe|zip)$" } | ForEach-Object {
    $sizeMB = [math]::Round($_.Length / 1MB, 2)
    Write-Host "  - $($_.FullName) ($sizeMB MB)"
}
