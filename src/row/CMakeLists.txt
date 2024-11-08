add_subdirectory(chord)
add_subdirectory(note)

target_sources(JustlyLibrary PRIVATE
    "InsertRow.hpp"
    "InsertRemoveRows.hpp"
    "RowsModel.hpp"
    "Row.cpp"
    "Row.hpp"
    "SetCell.hpp"
    "SetCells.hpp"
)