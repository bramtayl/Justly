target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/Chord.h"
    "justly/Instrument.h"
    "justly/Interval.h"
    "justly/Note.h"
    "justly/NoteChord.h"
    "justly/NoteChordField.h"
    "justly/SongEditor.h"
    "justly/SongIndex.h"
    "justly/Song.h"
    "justly/StartingField.h"
    "justly/TreeLevel.h"
)