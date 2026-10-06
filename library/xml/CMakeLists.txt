
target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "XMLChildren.hpp"
    "XMLDocument.hpp"
    "XMLValidator.hpp"
    "ZipArchive.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "XMLDocument.cpp"
    "ZipArchive.cpp"
)
