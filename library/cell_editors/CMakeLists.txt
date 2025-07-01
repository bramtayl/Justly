set(EDITOR_HEADERS 
    "IntervalEditor.hpp"
    "PercussionInstrumentEditor.hpp"
    "ProgramEditor.hpp"
    "RationalEditor.hpp"
)

target_sources(JustlyLibrary INTERFACE FILE_SET justly_headers FILES
    ${EDITOR_HEADERS}
)

# need to include because Q_OBJECT compiles code
target_sources(Justly PRIVATE ${EDITOR_HEADERS})

if (BUILD_TESTS)
    target_sources(JustlyTests PRIVATE ${EDITOR_HEADERS})
endif()