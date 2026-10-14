target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "FileOpener.hpp"
    "XMLChildren.hpp"
    "XMLDocument.hpp"
    "XMLValidator.hpp"
    "musicxml_import.hpp"
    "song_file.hpp"
    "xml_helpers.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "FileOpener.cpp"
    "XMLDocument.cpp"
    "musicxml_import.cpp"
    "song_file.cpp"
    "xml_helpers.cpp"
)
