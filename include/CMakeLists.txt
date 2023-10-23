target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_headers
    TYPE HEADERS
    FILES
    "justly/editors/InstrumentEditor.h"

    "justly/editors/SongEditor.h"
    "justly/main/Player.h"
    "justly/main/Song.h"

    "justly/metatypes/Instrument.h"
    "justly/metatypes/Interval.h"

    "justly/models/ChordsModel.h"

    "justly/notechord/Note.h"
    "justly/notechord/NoteChord.h"
    "justly/notechord/Chord.h"

    "justly/test/Tester.h"

    "justly/utilities/SongIndex.h"
)