target_include_directories(JustlyTests PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

target_sources(JustlyTests PRIVATE
    "Tester.cpp"
    # need to include because Q_OBJECT compiles code
    "Tester.hpp"
)

target_link_libraries(JustlyTests PRIVATE
    JustlyLibrary
    Qt6::Test
)
