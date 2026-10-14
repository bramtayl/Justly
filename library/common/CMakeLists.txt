target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "helpers.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "helpers.cpp"
)
