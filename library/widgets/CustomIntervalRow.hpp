#pragma once

#include <QtWidgets/QWidget>

class QComboBox;
class QGridLayout;
class QPushButton;
class QUndoStack;
struct IntervalEditor;
struct SwitchTable;

// like IntervalRow, but the user picks the interval, either by typing it or
// from a combo box of named presets underneath
struct CustomIntervalRow : public QWidget {
  QUndoStack& undo_stack;
  SwitchTable& switch_table;
  QGridLayout& row_layout;
  QPushButton& minus_button;
  IntervalEditor& interval_editor;
  QPushButton& plus_button;
  QComboBox& presets_box;

  CustomIntervalRow(QUndoStack& undo_stack_input,
                    SwitchTable& switch_table_input);
};
