add_subdirectory(chord)
add_subdirectory(note)

target_sources(JustlyLibrary PRIVATE
    "RowsModel.hpp"
    "Row.cpp"
    "Row.hpp"
)