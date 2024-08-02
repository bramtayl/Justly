target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/Instrument.hpp"
    "justly/Interval.hpp"
    "justly/JUSTLY_EXPORT.hpp"
    "justly/NoteChordColumn.hpp"
    "justly/Rational.hpp"
    "justly/SongEditor.hpp"
)