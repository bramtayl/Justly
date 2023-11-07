cmake -S . -B build -DBUILD_TESTS=ON -DTRACK_COVERAGE=ON
cmake --build build
#lcov --capture --initial --directory build --include "$(pwd)/**" --exclude "$(pwd)/build/**" --output-file coverage/uncovered.info
ctest --test-dir build
gcovr --exclude "build/.*" --html-nested coverage/coverage.html
# lcov --capture --directory build --include "$(pwd)/**" --exclude "$(pwd)/build/**" --output-file coverage/covered.info
# lcov --add-tracefile coverage/uncovered.info --add-tracefile coverage/covered.info --output-file coverage/lcov.info
# genhtml coverage/lcov.info --output-directory coverage/report
