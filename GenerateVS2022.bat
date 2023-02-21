mkdir Generated
cd Generated
cmake -DGLFW_USE_HYBRID_HPG:BOOL=ON -Wno-dev -G "Visual Studio 17 2022" -Ax64 ..
pause
