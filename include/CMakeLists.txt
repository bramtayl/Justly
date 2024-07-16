target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/ChangeId.hpp"
    "justly/Chord.hpp"
    "justly/ChordsModel.hpp"
    "justly/ChordsView.hpp"
    "justly/Instrument.hpp"
    "justly/InstrumentEditor.hpp"
    "justly/Interval.hpp"
    "justly/public_constants.hpp"
    "justly/Note.hpp"
    "justly/NoteChord.hpp"
    "justly/NoteChordField.hpp"
    "justly/Rational.hpp"
    "justly/SongEditor.hpp"
    "justly/TreeLevel.hpp"
)