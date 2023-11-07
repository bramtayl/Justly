target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/Chord.h"
    "justly/Interval.h"
    "justly/Note.h"
    "justly/NoteChord.h"
    "justly/NoteChordField.h"
    "justly/SongEditor.h"
    "justly/Song.h"
    "justly/StartingFieldId.h"
    "justly/Tester.h"
    "justly/TreeLevel.h"
)