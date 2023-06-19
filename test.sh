lcov --capture --directory build --output-file coverage/lcov.info
lcov --extract coverage/lcov.info '/home/brandon/Justly/src/*' --output-file coverage/lcov.info