
target_sources(JustlyLibrary PRIVATE FILE_SET justly_headers FILES 
    "ControlsColumn.hpp"
    "IntervalRow.hpp"
    "SongWidget.hpp"
    "SpinBoxes.hpp"
    "SwitchColumn.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "SongWidget.cpp"
)
