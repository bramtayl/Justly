#include "menus/SongMenuBar.hpp"

SongMenuBar::SongMenuBar(WindowBody& window_body)
    : file_menu(window_body), edit_menu(window_body), play_menu(window_body) {
  addMenu(&file_menu);
  addMenu(&edit_menu);
  addMenu(&view_menu);
  addMenu(&play_menu);
}
