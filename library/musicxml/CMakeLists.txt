target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "MusicXMLImport.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "MusicXMLImport.cpp"
)
