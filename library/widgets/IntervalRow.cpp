#include "widgets/IntervalRow.hpp"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "actions/SetCells.hpp"
#include "cell_editors/IntervalEditor.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

namespace {

struct IntervalLimit {
  const char* name;
  int value;
  int maximum;
  const char* comparison;
};

auto check_interval(QWidget& parent_widget, const Interval& interval) -> bool {
  for (const auto& limit :
       {IntervalLimit{.name = "Numerator",
                      .value = interval.ratio.numerator,
                      .maximum = MAX_NUMERATOR,
                      .comparison = " greater than maximum "},
        IntervalLimit{.name = "Denominator",
                      .value = interval.ratio.denominator,
                      .maximum = MAX_DENOMINATOR,
                      .comparison = " greater than maximum "},
        IntervalLimit{.name = "Octave",
                      .value = interval.octave,
                      .maximum = MAX_OCTAVE,
                      .comparison = " (absolutely) greater than maximum "}}) {
    if (std::abs(limit.value) > limit.maximum) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr(limit.name) << " " << limit.value
             << QObject::tr(limit.comparison) << limit.maximum;
      QMessageBox::warning(&parent_widget,
                           QObject::tr("%1 error").arg(QObject::tr(limit.name)),
                           message);
      return false;
    }
  }
  return true;
}

// multiplies the interval column of each selected row by interval, or returns
// nullptr (after warning) if any result would be out of range
template <RowInterface SubRow>
[[nodiscard]] auto make_update_interval_command(
    QWidget& parent_widget, RowsModel<SubRow>& rows_model,
    const QItemSelectionRange& range, const int interval_column,
    const Interval& interval) -> QUndoCommand* {
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);
  auto new_rows =
      copy_items(rows_model.get_rows(), first_row_number, number_of_rows);
  for (auto& row : new_rows) {
    const auto new_interval = row.interval * interval;
    if (!check_interval(parent_widget, new_interval)) {
      return nullptr;
    }
    row.interval = new_interval;
  }
  return new SetCells(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number, number_of_rows, interval_column,
      interval_column, std::move(new_rows));
}

}  // namespace

void update_interval(QUndoStack& undo_stack, SwitchTable& switch_table,
                     const Interval& interval) {
  const auto& range = get_only_range(switch_table);

  QUndoCommand* undo_command = nullptr;
  switch (switch_table.delegate.current_row_type) {
    case RowType::chord_type:
      undo_command = make_update_interval_command(
          switch_table, switch_table.chords_model, range,
          static_cast<int>(ChordColumn::chord_interval_column), interval);
      break;
    case RowType::pitched_note_type:
      undo_command = make_update_interval_command(
          switch_table, switch_table.pitched_notes_model, range,
          static_cast<int>(PitchedNoteColumn::pitched_note_interval_column),
          interval);
      break;
    case RowType::unpitched_note_type:
    case RowType::pitched_voice_type:
    case RowType::unpitched_voice_type:
      // interval rows are disabled for these row types; see
      // ReplaceTable.cpp's update_actions/set_interval_rows_are_enabled
      Q_UNREACHABLE();
  }
  if (undo_command != nullptr) {
    undo_stack.push(undo_command);
  }
}

void make_square(QPushButton& button) {
  button.setFixedWidth(button.sizeHint().height());
}

IntervalRow::IntervalRow(QUndoStack& undo_stack_input,
                         SwitchTable& switch_table_input,
                         const char* const interval_name,
                         Interval interval_input)
    : undo_stack(undo_stack_input),
      switch_table(switch_table_input),
      minus_button(*(new QPushButton("−", this))),
      plus_button(*(new QPushButton("+", this))),
      interval(interval_input) {
  make_square(minus_button);
  make_square(plus_button);
  auto& row_layout =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QHBoxLayout(this));
  row_layout.addWidget(&minus_button);
  row_layout.addWidget(
      new QLabel(interval_name));  // NOLINT(cppcoreguidelines-owning-memory)
  row_layout.addWidget(&plus_button);

  QObject::connect(
      &minus_button, &QPushButton::released, this, [this]() -> auto {
        update_interval(undo_stack, switch_table, Interval() / interval);
      });

  QObject::connect(&plus_button, &QPushButton::released, this,
                   [this]() -> auto {
                     update_interval(undo_stack, switch_table, interval);
                   });
}
