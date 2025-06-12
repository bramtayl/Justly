
target_sources(JustlyLibrary PRIVATE FILE_SET justly_headers FILES 
    "EditMenu.hpp"
    "FileMenu.hpp"
    "InsertMenu.hpp"
    "PasteMenu.hpp"
    "PlayMenu.hpp"
    "SongMenuBar.hpp"
    "ViewMenu.hpp"
)

target_sources(JustlyLibrary PRIVATE 
    "SongMenuBar.cpp"
)
