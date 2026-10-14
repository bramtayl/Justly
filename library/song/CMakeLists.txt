target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "Song.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "Song.cpp"
)
