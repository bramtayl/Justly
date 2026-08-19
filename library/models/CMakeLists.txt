target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "ChordsModel.hpp"
    "PitchedNotesModel.hpp"
    "PitchedVoicesModel.hpp"
    "UnpitchedVoicesModel.hpp"
    "RowsModel.hpp"
    "UndoRowsModel.hpp"
    "UnpitchedNotesModel.hpp"
    "VoicesModel.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "ChordsModel.cpp"
    "PitchedNotesModel.cpp"
    "PitchedVoicesModel.cpp"
    "RowsModel.cpp"
    "UnpitchedNotesModel.cpp"
    "UnpitchedVoicesModel.cpp"
)
