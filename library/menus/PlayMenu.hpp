#pragma once

#include <QtWidgets/QMenu>

#include "other/TableSelection.hpp"

struct WindowBody;

[[nodiscard]] auto get_play_selection(const WindowBody& window_body)
    -> TableSelection;

struct PlayMenu : public QMenu {
  QAction play_action;
  QAction play_to_end_action;
  QAction stop_playing_action;

  explicit PlayMenu(WindowBody& window_body);
};
