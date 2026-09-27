@echo off
REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cl /EHsc /std:c++17 test_barrier.cpp /I ..\include /Fe:test_barrier.exe
echo Done.
