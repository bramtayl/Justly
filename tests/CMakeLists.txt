target_include_directories(JustlyTests PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

target_sources(JustlyTests PRIVATE
    "Tester.cpp"
    "cell_type_tests.cpp"
    "export_tests.cpp"
    "io_tests.cpp"
    "musicxml_tests.cpp"
    "paste_tests.cpp"
    "piano_roll_tests.cpp"
    "playback_tests.cpp"
    "save_recovery_tests.cpp"
    "table_tests.cpp"
    "voice_tests.cpp"
    # need to include because Q_OBJECT compiles code
    "Tester.hpp"
    "test_helpers.hpp"
)

target_link_libraries(JustlyTests PRIVATE
    JustlyLibrary
    Qt6::Test
)
