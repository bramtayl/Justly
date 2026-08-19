#pragma once

#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/QtAssert>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QUndoStack>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <cstdlib>
#include <utility>

#include "actions/SetCells.hpp"
#include "cell_editors/IntervalEditor.hpp"
#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/RowsModel.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/RowType.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"

[[nodiscard]] auto check_interval(QWidget &parent_widget,
                                  const Interval &interval) -> bool;

void update_interval(QUndoStack &undo_stack, SwitchTable &switch_table,
                     const Interval &interval);

void make_square(QPushButton &button);

struct IntervalRow : public QWidget {
  QUndoStack &undo_stack;
  SwitchTable &switch_table;
  QBoxLayout &row_layout = *(new QHBoxLayout(this));
  QPushButton &minus_button = *(new QPushButton("−", this));
  QLabel &text;
  QPushButton &plus_button = *(new QPushButton("+", this));
  const Interval interval;

  IntervalRow(QUndoStack &undo_stack_input, SwitchTable &switch_table_input,
              const char *const interval_name, Interval interval_input);
};

void set_interval_row_is_enabled(IntervalRow &interval_row,
                                 bool is_enabled);

void set_interval_rows_is_enabled(IntervalRow &third_row,
                                  IntervalRow &fifth_row,
                                  IntervalRow &seventh_row,
                                  IntervalRow &octave_row,
                                  bool is_enabled);
