target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "EditMenu.hpp"
    "FileMenu.hpp"
    "InsertMenu.hpp"
    "MenuAction.hpp"
    "PasteMenu.hpp"
    "PlayMenu.hpp"
    "SongMenuBar.hpp"
    "ViewMenu.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "EditMenu.cpp"
    "FileMenu.cpp"
    "InsertMenu.cpp"
    "MenuAction.cpp"
    "PasteMenu.cpp"
    "PlayMenu.cpp"
    "SongMenuBar.cpp"
    "ViewMenu.cpp"
)
