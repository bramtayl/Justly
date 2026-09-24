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

void update_interval(QUndoStack& undo_stack, SwitchTable& switch_table,
                     const Interval& interval);

void make_square(QPushButton& button);
