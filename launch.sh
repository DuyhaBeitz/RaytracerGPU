clear
rm -R ./build/RTX_GPU
mkdir -p build
cd build
cmake ..
make
cd ..
cp -R ./assets build/RTX_GPU/assets
./build/RTX_GPU/RTX_GPU