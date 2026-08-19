#pragma once

#include <QtGui/QAction>
#include <QtWidgets/QMenu>

struct ViewMenu : public QMenu {
  QAction back_to_chords_action = QAction(ViewMenu::tr("&Back to chords"));
  QAction edit_pitched_voices_action = QAction(ViewMenu::tr("&Pitched voices"));
  QAction edit_unpitched_voices_action = QAction(ViewMenu::tr("&Unpitched voices"));
  QAction previous_chord_action = QAction(ViewMenu::tr("&Previous chord"));
  QAction next_chord_action = QAction(ViewMenu::tr("&Next chord"));
  QAction show_piano_roll_action = QAction(ViewMenu::tr("&Piano roll"));
  QAction zoom_in_action = QAction(ViewMenu::tr("Zoom &in"));
  QAction zoom_out_action = QAction(ViewMenu::tr("Zoom &out"));

  explicit ViewMenu();
};
