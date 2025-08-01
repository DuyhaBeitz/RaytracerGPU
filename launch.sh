clear
rm -R ./build/RTX_GPU
mkdir -p build
cd build
cmake ..
make
cd ..
./build/RTX_GPU/RTX_GPU