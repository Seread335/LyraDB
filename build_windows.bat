@echo off
REM Build script for Windows (MSVC)

setlocal enabledelayedexpansion

echo.
echo Building LyraDB for Windows...
echo.

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set ARCH=%2
if "%ARCH%"=="" set ARCH=x64

REM Create build directory
if not exist "build_windows_%ARCH%" mkdir "build_windows_%ARCH%"
cd "build_windows_%ARCH%"

REM Generate project
echo Generating CMake project...
cmake .. -G "Visual Studio 17 2022" -A %ARCH% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_REST_SERVER=ON ^
    -DBUILD_TESTS=ON ^
    -DENABLE_SIMD=ON

if errorlevel 1 (
    echo CMake generation failed!
    cd ..
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config %BUILD_TYPE% -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo Build failed!
    cd ..
    exit /b 1
)

REM Package
echo Creating distribution package...
if not exist "..\dist\windows_%ARCH%" mkdir "..\dist\windows_%ARCH%"

copy "%BUILD_TYPE%\lyradb.dll" "..\dist\windows_%ARCH%\" >nul
copy "%BUILD_TYPE%\lyradb.lib" "..\dist\windows_%ARCH%\" >nul
copy "..\include\lyradb_c.h" "..\dist\windows_%ARCH%\" >nul
copy "..\include\lyradb.h" "..\dist\windows_%ARCH%\" >nul 2>&1

cd ..

echo.
echo Windows build complete!
echo   Output: dist\windows_%ARCH%\
echo   - lyradb.dll (shared library)
echo   - lyradb.lib (import library)
echo   - lyradb_c.h (C API header)
echo.

endlocal
