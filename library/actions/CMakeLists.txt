target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "AffectedVoiceNote.hpp"
    "ChangeId.hpp"
    "DeleteCells.hpp"
    "InsertRemoveRows.hpp"
    "InsertRow.hpp"
    "InsertVoiceRow.hpp"
    "RemoveVoiceRows.hpp"
    "RenumberedVoiceNote.hpp"
    "ReplaceTable.hpp"
    "SetCell.hpp"
    "SetCells.hpp"
    "SetDouble.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "AffectedVoiceNote.cpp"
    "ReplaceTable.cpp"
    "SetDouble.cpp"
)
