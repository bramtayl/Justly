# use gcc not clang
# make sure BUILD_TESTS=ON and TRACK_COVERAGE=ON
gcovr --exclude "$(pwd)/src/" --xml coverage/coverage.xml
