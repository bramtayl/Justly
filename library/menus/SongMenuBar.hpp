#pragma once

#include <QtWidgets/QMenuBar>

#include "menus/EditMenu.hpp"
#include "menus/FileMenu.hpp"
#include "menus/PlayMenu.hpp"
#include "menus/ViewMenu.hpp"

struct SongWidget;

struct SongMenuBar : public QMenuBar {
  FileMenu file_menu;
  EditMenu edit_menu;
  ViewMenu view_menu;
  PlayMenu play_menu;

  explicit SongMenuBar(SongWidget &song_widget)
      : file_menu(FileMenu(song_widget)), edit_menu(EditMenu(song_widget)),
        play_menu(PlayMenu(song_widget)) {
    addMenu(&file_menu);
    addMenu(&edit_menu);
    addMenu(&view_menu);
    addMenu(&play_menu);
  }
};
