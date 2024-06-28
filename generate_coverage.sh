# use gcc not clang
# make sure BUILD_TESTS=ON and TRACK_COVERAGE=ON
gcovr --exclude "build/.*" --xml coverage/coverage.xml
