target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "MusicXMLImport.hpp"
    "MusicXMLPart.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "MusicXMLImport.cpp"
    "MusicXMLPart.cpp"
)
