#pragma once

#include <QtWidgets/QMenu>

#include "widgets/SongWidget.hpp"

struct ViewMenu : public QMenu {
  QAction back_to_chords_action = QAction(ViewMenu::tr("&Back to chords"));
  QAction previous_chord_action = QAction(ViewMenu::tr("&Previous chord"));
  QAction next_chord_action = QAction(ViewMenu::tr("&Next chord"));

  explicit ViewMenu() : QMenu(ViewMenu::tr("&View")) {
    add_menu_action(*this, back_to_chords_action, QKeySequence::Back, false);
    add_menu_action(*this, previous_chord_action, QKeySequence::PreviousChild,
                    false);
    add_menu_action(*this, next_chord_action, QKeySequence::NextChild, false);
  }
};
