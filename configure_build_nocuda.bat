@echo off
setlocal
REM Simple wrapper to configure & build without CUDA
pushd %~dp0
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist build mkdir build
pushd build
cmake .. -G "NMake Makefiles" -DBUILD_CUDA=OFF -DBUILD_TESTS=ON
cmake --build .
popd
popd
echo Build complete.
endlocal
