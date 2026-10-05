target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "ChordsModel.hpp"
    "NotesModel.hpp"
    "RowsModel.hpp"
    "UndoRowsModel.hpp"
    "VoicesModel.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "ChordsModel.cpp"
)
