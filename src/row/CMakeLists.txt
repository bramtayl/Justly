add_subdirectory(chord)
add_subdirectory(pitched_note)
add_subdirectory(unpitched_note)

target_sources(JustlyLibrary PRIVATE
    "InsertRow.hpp"
    "InsertRemoveRows.hpp"
    "RowsModel.hpp"
    "Row.cpp"
    "Row.hpp"
    "SetCell.hpp"
    "SetCells.hpp"
)