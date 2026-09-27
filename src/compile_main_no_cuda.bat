@echo off
REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Compile CPU-only sources (exclude CUDA) for a quick run
cl /EHsc /std:c++17 main.cpp MonteCarlo.cpp Random.cpp /I ..\include /Fe:MonteCarloMain.exe
echo Built MonteCarloMain.exe (CPU-only)
