#pragma once

#include <QtGui/QKeySequence>

class QAction;
class QMenu;

void add_menu_action(
    QMenu& menu, QAction& action,
    QKeySequence::StandardKey key_sequence = QKeySequence::UnknownKey,
    bool enabled = true);
