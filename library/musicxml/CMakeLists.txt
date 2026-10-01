target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "Accidental.hpp"
    "MusicXMLChord.hpp"
    "MusicXMLMeasure.hpp"
    "MusicXMLNote.hpp"
    "MusicXMLPart.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "MusicXMLPart.cpp"
)
