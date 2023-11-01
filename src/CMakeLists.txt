add_subdirectory("commands")
add_subdirectory("editors")
add_subdirectory("metatypes")
add_subdirectory("models")
add_subdirectory("notechord")
add_subdirectory("test")
add_subdirectory("utilities")


target_sources(JustlyLibrary PRIVATE
    "ChordsDelegate.h"
    "ChordsDelegate.cpp"
    "ChordsView.h"
    "ChordsView.cpp"
    "Player.cpp"
    "Song.cpp"
)