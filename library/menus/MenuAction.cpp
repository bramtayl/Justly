#include "menus/MenuAction.hpp"

#include <QtWidgets/QMenu>

void add_menu_action(QMenu& menu, QAction& action,
                     const QKeySequence::StandardKey key_sequence,
                     const bool enabled) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}
