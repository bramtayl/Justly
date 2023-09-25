#!/bin/bash
lcov --capture --initial --include "$(pwd)/src/**" --directory build --output-file /tmp/justly_base.info
build/JustlyTests
lcov --capture --include "$(pwd)/src/**" --directory build --output-file /tmp/justly_covered.info
lcov --add-tracefile /tmp/justly_base.info --add-tracefile /tmp/justly_covered.info --output-file coverage/lcov.info
genhtml coverage/lcov.info --output-directory coverage/report