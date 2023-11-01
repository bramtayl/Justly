target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_public_headers
    TYPE HEADERS
    FILES
    "justly/editors/InstrumentEditor.h"

    "justly/editors/SongEditor.h"
    "justly/Player.h"
    "justly/Song.h"

    "justly/metatypes/Instrument.h"
    "justly/metatypes/Interval.h"

    "justly/models/ChordsModel.h"

    "justly/notechord/Note.h"
    "justly/notechord/NoteChord.h"
    "justly/notechord/Chord.h"

    "justly/test/Tester.h"

    "justly/utilities/SongIndex.h"
)