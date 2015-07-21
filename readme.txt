Building libgit2:

cd .\ext\libgit2
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=OFF -DSTATIC_CRT=OFF
cmake --build . --config Debug
cmake --build . --config Release