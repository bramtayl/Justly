rm --recursive --force build
mkdir "build"
cmake -S "." -B "build" -DCMAKE_BUILD_TYPE="Release" -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build "build" --config "Release" --target "JustlyTests"
cmake --build "build" --config "Release" --target "install"