target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/JUSTLY_EXPORT.hpp"
    "justly/ChordColumn.hpp"
    "justly/NoteColumn.hpp"
    "justly/PercussionColumn.hpp"
    "justly/justly.hpp"
)