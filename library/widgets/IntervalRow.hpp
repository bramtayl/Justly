#pragma once

#include <QtWidgets/QWidget>

#include "cell_types/Interval.hpp"

class QBoxLayout;
class QLabel;
class QPushButton;
class QUndoStack;
struct SwitchTable;

struct IntervalRow : public QWidget {
  QUndoStack& undo_stack;
  SwitchTable& switch_table;
  QBoxLayout& row_layout;
  QPushButton& minus_button;
  QLabel& text;
  QPushButton& plus_button;
  const Interval interval;

  IntervalRow(QUndoStack& undo_stack_input, SwitchTable& switch_table_input,
              const char* interval_name, Interval interval_input);
};

void set_interval_rows_is_enabled(IntervalRow& third_row,
                                  IntervalRow& fifth_row,
                                  IntervalRow& seventh_row,
                                  IntervalRow& octave_row, bool is_enabled);
