call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /std:c++20 /EHsc /I include src\main.cpp src\MonteCarlo.cpp src\Random.cpp /Fe:MonteCarloEngine_v2.exe
