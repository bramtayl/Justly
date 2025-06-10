#pragma once

#include "SetCells.hpp"
#include "SwitchColumn.hpp"

[[nodiscard]] static auto check_interval(QWidget &parent_widget,
                                         const Interval &interval) -> bool {
  const auto numerator = interval.ratio.numerator;
  const auto denominator = interval.ratio.denominator;
  const auto octave = interval.octave;
  if (std::abs(numerator) > MAX_THREE_DIGITS) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Numerator ") << numerator
           << QObject::tr(" greater than maximum ") << MAX_THREE_DIGITS;
    QMessageBox::warning(&parent_widget, QObject::tr("Numerator error"),
                         message);
    return false;
  }
  if (std::abs(denominator) > MAX_THREE_DIGITS) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Denominator ") << denominator
           << QObject::tr(" greater than maximum ") << MAX_THREE_DIGITS;
    QMessageBox::warning(&parent_widget, QObject::tr("Denominator error"),
                         message);
    return false;
  }
  if (std::abs(octave) > MAX_OCTAVE) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Octave ") << octave
           << QObject::tr(" (absolutely) greater than maximum ") << MAX_OCTAVE;
    QMessageBox::warning(&parent_widget, QObject::tr("Octave error"), message);
    return false;
  }
  return true;
}

static void update_interval(QUndoStack &undo_stack, SwitchColumn &switch_column,
                            const Interval &interval) {
  const auto &range = get_only_range(switch_column);
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);

  const auto current_row_type = switch_column.current_row_type;
  QUndoCommand *undo_command = nullptr;
  if (current_row_type == chord_type) {
    auto &chords_model = switch_column.chords_table.model;
    auto new_chords =
        copy_items(chords_model.get_rows(), first_row_number, number_of_rows);
    for (auto &chord : new_chords) {
      const auto new_interval = chord.interval * interval;
      if (!check_interval(switch_column, new_interval)) {
        return;
      }
      chord.interval = new_interval;
    }
    undo_command = make_set_cells_command(
        chords_model, first_row_number, number_of_rows, chord_interval_column,
        chord_interval_column, std::move(new_chords));
  } else if (current_row_type == pitched_note_type) {
    auto &pitched_notes_model = switch_column.pitched_notes_table.model;
    auto new_pitched_notes = copy_items(pitched_notes_model.get_rows(),
                                        first_row_number, number_of_rows);
    for (auto &pitched_note : new_pitched_notes) {
      const auto new_interval = pitched_note.interval * interval;
      if (!check_interval(switch_column, new_interval)) {
        return;
      }
      pitched_note.interval = new_interval;
    }
    undo_command = make_set_cells_command(
        pitched_notes_model, first_row_number, number_of_rows,
        pitched_note_interval_column, pitched_note_interval_column,
        std::move(new_pitched_notes));
  } else {
    Q_ASSERT(false);
  }
  undo_stack.push(undo_command);
}

static void make_square(QPushButton &button) {
  button.setFixedWidth(button.sizeHint().height());
}

struct IntervalRow : public QWidget {
  QUndoStack &undo_stack;
  SwitchColumn &switch_column;
  QBoxLayout &row_layout = *(new QHBoxLayout(this));
  QPushButton &minus_button = *(new QPushButton("−", this));
  QLabel &text;
  QPushButton &plus_button = *(new QPushButton("+", this));
  const Interval interval;

  IntervalRow(QUndoStack &undo_stack_input, SwitchColumn &switch_column_input,
              const char *const interval_name, Interval interval_input)
      : undo_stack(undo_stack_input), switch_column(switch_column_input),
        text(*(new QLabel(interval_name, this))), interval(interval_input) {
    make_square(minus_button);
    make_square(plus_button);
    row_layout.addWidget(&minus_button);
    row_layout.addWidget(&text);
    row_layout.addWidget(&plus_button);

    auto &switch_column_ref = this->switch_column;
    auto &undo_stack_ref = this->undo_stack;
    const auto &interval_ref = this->interval;

    QObject::connect(&minus_button, &QPushButton::released, this,
                     [&undo_stack_ref, &switch_column_ref, &interval_ref]() {
                       update_interval(undo_stack_ref, switch_column_ref,
                                       Interval() / interval_ref);
                     });

    QObject::connect(&plus_button, &QPushButton::released, this,
                     [&undo_stack_ref, &switch_column_ref, &interval_ref]() {
                       update_interval(undo_stack_ref, switch_column_ref,
                                       interval_ref);
                     });
  }
};

static void set_interval_row_enabled(IntervalRow &interval_row, bool enabled) {
  interval_row.minus_button.setEnabled(enabled);
  interval_row.plus_button.setEnabled(enabled);
}

static inline void set_interval_rows_enabled(IntervalRow &third_row, IntervalRow &fifth_row,
                             IntervalRow &seventh_row, IntervalRow &octave_row,
                             bool enabled) {
  set_interval_row_enabled(third_row, enabled);
  set_interval_row_enabled(fifth_row, enabled);
  set_interval_row_enabled(seventh_row, enabled);
  set_interval_row_enabled(octave_row, enabled);
}
