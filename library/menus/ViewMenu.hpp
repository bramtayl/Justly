#pragma once

#include <QtGui/QAction>
#include <QtWidgets/QMenu>

struct ViewMenu : public QMenu {
  QAction back_to_chords_action;
  QAction edit_pitched_voices_action;
  QAction edit_unpitched_voices_action;
  QAction previous_chord_action;
  QAction next_chord_action;
  QAction show_piano_roll_action;
  QAction zoom_in_action;
  QAction zoom_out_action;

  explicit ViewMenu();
};
