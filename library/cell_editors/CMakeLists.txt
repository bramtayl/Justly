set(EDITOR_HEADERS 
    "IntervalEditor.hpp"
    "RationalEditor.hpp"
    "StringPicker.hpp"
)

target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    ${EDITOR_HEADERS}
)

target_sources(JustlyLibrary PRIVATE
    "IntervalEditor.cpp"
    "RationalEditor.cpp"
    "StringPicker.cpp"
)

# need to include because Q_OBJECT compiles code
target_sources(Justly PRIVATE ${EDITOR_HEADERS})

if (BUILD_TESTS)
    target_sources(JustlyTests PRIVATE ${EDITOR_HEADERS})
endif()