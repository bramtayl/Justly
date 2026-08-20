#include "menus/SongMenuBar.hpp"

SongMenuBar::SongMenuBar(SongWidget& song_widget)
    : file_menu(FileMenu(song_widget)),
      edit_menu(EditMenu(song_widget)),
      play_menu(PlayMenu(song_widget)) {
  addMenu(&file_menu);
  addMenu(&edit_menu);
  addMenu(&view_menu);
  addMenu(&play_menu);
}
