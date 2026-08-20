#pragma once

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QPushButton>

#include "cell_types/Interval.hpp"

class QLabel;
class QUndoStack;
struct SwitchTable;

struct IntervalRow : public QWidget {
  QUndoStack& undo_stack;
  SwitchTable& switch_table;
  QBoxLayout& row_layout = *(new QHBoxLayout(this));
  QPushButton& minus_button = *(new QPushButton("−", this));
  QLabel& text;
  QPushButton& plus_button = *(new QPushButton("+", this));
  const Interval interval;

  IntervalRow(QUndoStack& undo_stack_input, SwitchTable& switch_table_input,
              const char* interval_name, Interval interval_input);
};

void set_interval_rows_is_enabled(IntervalRow& third_row,
                                  IntervalRow& fifth_row,
                                  IntervalRow& seventh_row,
                                  IntervalRow& octave_row, bool is_enabled);
