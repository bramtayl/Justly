
target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "ControlsColumn.hpp"
    "IntervalRow.hpp"
    "SongWidget.hpp"
    "SpinBoxes.hpp"
    "SongEditor.hpp"
    "SwitchColumn.hpp"
    "SwitchDelegate.hpp"
    "SwitchTable.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "ControlsColumn.cpp"
    "IntervalRow.cpp"
    "SongWidget.cpp"
    "SpinBoxes.cpp"
    "SongEditor.cpp"
    "SwitchColumn.cpp"
    "SwitchDelegate.cpp"
    "SwitchTable.cpp"
)

add_subdirectory("piano_roll")
