target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/constants.hpp"
    "justly/global.hpp"
    "justly/Chord.hpp"
    "justly/Instrument.hpp"
    "justly/InstrumentEditor.hpp"
    "justly/Interval.hpp"
    "justly/IntervalEditor.hpp"
    "justly/Note.hpp"
    "justly/NoteChord.hpp"
    "justly/NoteChordField.hpp"
    "justly/SongEditor.hpp"
    "justly/Song.hpp"
    "justly/TreeLevel.hpp"
)