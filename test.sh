mkdir "Release"
cmake -S "." -B "./Release" -DCMAKE_BUILD_TYPE="Release" -DCMAKE_TOOLCHAIN_FILE="~/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build "./Release" --config "Release" --target "Justly"
mkdir "Debug"
cmake -S "." -B "./Debug" -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_TOOLCHAIN_FILE="~/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build "./Debug" --config "Debug" --target "Justly"
cpack --config "MultiCpackConfig.cmake"

lcov --capture --directory build --output-file coverage/lcov.info
lcov --extract coverage/lcov.info '/home/brandon/Justly/src/*' --output-file coverage/lcov.info