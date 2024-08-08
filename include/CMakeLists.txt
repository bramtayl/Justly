target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/get_instrument_pointer.hpp"
    "justly/JUSTLY_EXPORT.hpp"
    "justly/NoteChordColumn.hpp"
    "justly/SongEditor.hpp"
)