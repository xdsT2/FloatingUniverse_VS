@echo off
echo ========================================
echo   FloatingUniverse 打包脚本
echo ========================================
echo.

:: 设置 Qt 路径（请根据实际情况修改）
:: 常见的 Qt 安装路径：
:: - C:\Qt\Qt5.x.x\5.x.x\msvc2019_64\bin
:: - C:\Qt\Qt5.x.x\5.x.x\msvc2017_64\bin
:: - D:\Qt\Qt5.x.x\5.x.x\msvc2019_64\bin
set QT_BIN_PATH=C:\Qt\Qt5.15.2\5.15.2\msvc2019_64\bin

echo 正在检查 Qt 路径...
if not exist "%QT_BIN_PATH%\qmake.exe" (
    echo [错误] 未找到 qmake.exe
    echo 请修改此脚本中的 QT_BIN_PATH 变量，设置为你本地 Qt 的 bin 目录路径
    echo.
    echo 常见的 Qt 路径示例：
    echo   C:\Qt\Qt5.15.2\5.15.2\msvc2019_64\bin
    echo   C:\Qt\5.15.2\msvc2019_64\bin
    pause
    exit /b 1
)

echo 正在检查编译器...
set COMPILER_FOUND=0

:: 检查 MSVC 编译器
where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo [成功] 找到 MSVC 编译器
    set COMPILER_FOUND=1
    goto :BUILD
)

:: 检查 MinGW 编译器
where g++ >nul 2>nul
if %errorlevel% equ 0 (
    echo [成功] 找到 MinGW 编译器
    set COMPILER_FOUND=1
    goto :BUILD
)

echo [错误] 未找到编译器
echo 请先打开 Qt Creator 或使用 Visual Studio 的开发者命令提示符
pause
exit /b 1

:BUILD
echo.
echo ========================================
echo   步骤 1: 使用 qmake 生成 Makefile
echo ========================================
"%QT_BIN_PATH%\qmake.exe" FloatingUniverse.pro
if %errorlevel% neq 0 (
    echo [错误] qmake 失败
    pause
    exit /b 1
)
echo [成功] qmake 完成

echo.
echo ========================================
echo   步骤 2: 编译项目
echo ========================================

:: 根据编译器选择 nmake 或 mingw32-make
where nmake >nul 2>nul
if %errorlevel% equ 0 (
    echo 使用 nmake 编译...
    nmake
) else (
    echo 使用 mingw32-make 编译...
    "%QT_BIN_PATH%\..\..\..\Tools\mingw810_64\bin\mingw32-make.exe"
)

if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)
echo [成功] 编译完成

echo.
echo ========================================
echo   步骤 3: 部署 Qt 依赖库
echo ========================================

:: 查找生成的 exe 文件
set EXE_PATH=release\FloatingUniverse.exe
if not exist "%EXE_PATH%" (
    set EXE_PATH=debug\FloatingUniverse.exe
)

if not exist "%EXE_PATH%" (
    echo [错误] 未找到生成的可执行文件
    pause
    exit /b 1
)

echo 正在部署依赖库...
"%QT_BIN_PATH%\windeployqt.exe" --release "%EXE_PATH%"
if %errorlevel% neq 0 (
    echo [警告] windeployqt 可能出现问题
)

echo.
echo ========================================
echo   打包完成！
echo ========================================
echo 可执行文件位置: %EXE_PATH%
echo 依赖库已自动部署
echo.
echo 提示：
echo 1. 如果需要打包成安装包，可以使用 Inno Setup 等工具
echo 2. 如果需要单文件版本，可以使用 Enigma Virtual Box 等工具
pause
