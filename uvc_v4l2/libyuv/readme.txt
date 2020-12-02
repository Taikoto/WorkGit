if this libyuv.a is not supported to your platform, you can download the libyuv source at the address() for cross compile
src doc is libyuv source file, you can cross compile these files based on your actual platform
for example:

git clone https://github.com/bilibili/libyuv.git
modify your cross compile in CMakeLists.txt

cd src/libyuv && mkdir -p build
cd build/ && cmake ../ && make
cp -rf libyuv.a ../../../

note:
remove src after build successfully and copy libyuv.a to ../../../
