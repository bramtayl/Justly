rm -rf build
cmake -B build
cmake --build build --config Release --target RunTests
cd build
ctest -C Release --output-on-failure
cd ..
lcov --capture --directory build --output-file coverage/lcov.info