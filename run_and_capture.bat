@echo off
echo Building project...
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DX9Sample.sln -p:Configuration=Debug -p:Platform=x64
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo Running and capturing output...
cd test
start /B DX9Sample.exe > output.txt 2>&1

echo Waiting 5 seconds for program to start...
timeout /t 5 /nobreak > nul

echo Killing process...
taskkill /F /IM DX9Sample.exe

echo Output saved to test\output.txt
cd ..
type test\output.txt
pause