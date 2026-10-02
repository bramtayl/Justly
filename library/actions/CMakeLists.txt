target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "ChangeId.hpp"
    "InsertRemoveRows.hpp"
    "NoteLocation.hpp"
    "OrphanedVoiceNameLocation.hpp"
    "RemoveVoiceRows.hpp"
    "RenameVoice.hpp"
    "ReplaceTable.hpp"
    "SetCells.hpp"
    "SetDouble.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "ReplaceTable.cpp"
    "SetDouble.cpp"
)
