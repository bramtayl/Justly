target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/public_constants.hpp"
    "justly/public_constants.hpp"
    "justly/Chord.hpp"
    "justly/Instrument.hpp"
    "justly/Interval.hpp"
    "justly/Note.hpp"
    "justly/NoteChord.hpp"
    "justly/NoteChordField.hpp"
    "justly/Rational.hpp"
    "justly/SongEditor.hpp"
    "justly/Song.hpp"
    "justly/SongIndex.hpp"
    "justly/TreeLevel.hpp"
)