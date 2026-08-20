#pragma once

#include <QtWidgets/QMenuBar>

#include "menus/EditMenu.hpp"
#include "menus/FileMenu.hpp"
#include "menus/PlayMenu.hpp"
#include "menus/ViewMenu.hpp"

struct SongMenuBar : public QMenuBar {
  FileMenu file_menu;
  EditMenu edit_menu;
  ViewMenu view_menu;
  PlayMenu play_menu;

  explicit SongMenuBar(SongWidget& song_widget);
};
