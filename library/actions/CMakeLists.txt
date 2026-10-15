target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "ChangeId.hpp"
    "InsertRemoveRows.hpp"
    "NoteLocation.hpp"
    "OrphanedVoiceNameLocation.hpp"
    "RemoveVoiceRows.hpp"
    "RenameVoice.hpp"
    "replace_table.hpp"
    "SetCells.hpp"
    "SetDouble.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "replace_table.cpp"
    "SetDouble.cpp"
)
