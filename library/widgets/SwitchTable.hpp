#pragma once

#include <QtCore/QSize>
#include <QtCore/QtAssert>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
#include <concepts>

#include "cell_editors/IntervalEditor.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/PitchedVoicesModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "models/UnpitchedVoicesModel.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Row.hpp"
#include "rows/RowType.hpp"
#include "widgets/SwitchDelegate.hpp"

class QUndoStack;
template <RowInterface SubRow> struct RowsModel;

static const auto WORDS_WIDTH = 200;

template <RowInterface SubRow>
static void set_model(QAbstractItemView &item_view,
                      RowsModel<SubRow> &rows_model) {
  item_view.setModel(&rows_model);
  rows_model.selection_model_pointer = item_view.selectionModel();
}

template <std::derived_from<QWidget> SubWidget>
[[nodiscard]] static auto get_minimum_size() -> const auto & {
  static const auto minimum_size = SubWidget(nullptr).minimumSizeHint();
  return minimum_size;
}

struct SwitchTable : public QTableView {
  ChordsModel chords_model;
  PitchedNotesModel pitched_notes_model;
  UnpitchedNotesModel unpitched_notes_model;
  PitchedVoicesModel pitched_voices_model;
  UnpitchedVoicesModel unpitched_voices_model;
  SwitchDelegate &delegate;
  SwitchTable(QUndoStack &undo_stack, Song &song);

  [[nodiscard]] auto sizeHint() const -> QSize override;

  [[nodiscard]] auto minimumSizeHint() const -> QSize override;
};

// dispatches on the current row type, calling visitor with a reference to
// the matching model member; Table may be SwitchTable or const SwitchTable
template <typename Table, typename Visitor>
static auto dispatch_row_type(Table &switch_table, Visitor visitor) {
  switch (switch_table.delegate.current_row_type) {
  case RowType::chord_type:
    return visitor(switch_table.chords_model);
  case RowType::pitched_note_type:
    return visitor(switch_table.pitched_notes_model);
  case RowType::unpitched_note_type:
    return visitor(switch_table.unpitched_notes_model);
  case RowType::pitched_voice_type:
    return visitor(switch_table.pitched_voices_model);
  case RowType::unpitched_voice_type:
    return visitor(switch_table.unpitched_voices_model);
  }
  Q_UNREACHABLE();
}
