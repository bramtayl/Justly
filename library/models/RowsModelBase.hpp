#pragma once

#include <QtCore/QAbstractTableModel>

// RowsModel is a template, so it can't declare signals itself
struct RowsModelBase : public QAbstractTableModel {
  Q_OBJECT

 signals:
  // asks to show this model in the switch table, pointed at the given chord's
  // notes (-1 for chords and voices), so undo/redo can show the rows they
  // change; MainWindow connects to this, since it owns everything
  // replace_table needs
  void show_rows_requested(int chord_number);
};
