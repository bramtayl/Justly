cmake -S . -B build -DBUILD_TESTS=ON -DTRACK_COVERAGE=ON
cmake --build build
ctest --test-dir build
gcovr --exclude "build/.*" --html-nested coverage/coverage.html
