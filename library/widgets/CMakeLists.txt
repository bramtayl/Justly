
target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "ControlsColumn.hpp"
    "CustomIntervalRow.hpp"
    "IntervalRow.hpp"
    "WindowBody.hpp"
    "SongFile.hpp"
    "SpinBoxes.hpp"
    "MainWindow.hpp"
    "SwitchColumn.hpp"
    "SwitchDelegate.hpp"
    "SwitchTable.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "ControlsColumn.cpp"
    "CustomIntervalRow.cpp"
    "IntervalRow.cpp"
    "WindowBody.cpp"
    "SongFile.cpp"
    "SpinBoxes.cpp"
    "MainWindow.cpp"
    "SwitchColumn.cpp"
    "SwitchDelegate.cpp"
    "SwitchTable.cpp"
)
