target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "ChangeId.hpp"
    "DeleteCells.hpp"
    "InsertRemoveRows.hpp"
    "InsertRow.hpp"
    "InsertVoiceRow.hpp"
    "OrphanedVoiceNumberLocation.hpp"
    "RemoveVoiceRows.hpp"
    "ReplaceTable.hpp"
    "SetCell.hpp"
    "SetCells.hpp"
    "SetDouble.hpp"
    "ShiftedVoiceNumberLocation.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "ReplaceTable.cpp"
    "SetDouble.cpp"
)
