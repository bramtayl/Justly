target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "Cells.hpp"
    "ChordsModel.hpp"
    "NotesModel.hpp"
    "RowsModel.hpp"
    "TableSelection.hpp"
    "UndoRowsModel.hpp"
    "VoicesModel.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "ChordsModel.cpp"
)
