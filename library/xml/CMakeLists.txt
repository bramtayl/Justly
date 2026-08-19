
target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "XMLDocument.hpp"
    "XMLParserContext.hpp"
    "XMLSchema.hpp"
    "XMLValidationContext.hpp"
    "XMLValidator.hpp"
    "ZipArchive.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "XMLDocument.cpp"
    "XMLParserContext.cpp"
    "XMLSchema.cpp"
    "XMLValidationContext.cpp"
    "ZipArchive.cpp"
)
