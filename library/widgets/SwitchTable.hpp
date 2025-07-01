#pragma once

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTableView>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/PercussionInstrumentEditor.hpp"
#include "cell_editors/ProgramEditor.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "other/helpers.hpp"
#include "rows/RowType.hpp"

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

[[nodiscard]] static inline auto
change_row_type(QTableView &table, const RowType row_type,
                ChordsModel &chords_model,
                PitchedNotesModel &pitched_notes_model,
                UnpitchedNotesModel &unpitched_notes_model) {
  const auto &interval_size = get_minimum_size<IntervalEditor>();
  const auto &rational_size = get_minimum_size<RationalEditor>();
  const auto &instrument_size = get_minimum_size<ProgramEditor>();
  const auto &percussion_instrument_size =
      get_minimum_size<PercussionInstrumentEditor>();
  const auto rational_width = rational_size.width();

  get_reference(table.verticalHeader())
      .setDefaultSectionSize(std::max(
          {instrument_size.height(), percussion_instrument_size.height(),
           rational_size.height(), interval_size.height()}));

  if (row_type == chord_type) {
    set_model(table, chords_model);

    table.setColumnWidth(chord_instrument_column, instrument_size.width());
    table.setColumnWidth(chord_percussion_instrument_column,
                         percussion_instrument_size.width());
    table.setColumnWidth(chord_interval_column, interval_size.width());
    table.setColumnWidth(chord_beats_column, rational_width);
    table.setColumnWidth(chord_velocity_ratio_column, rational_width);
    table.setColumnWidth(chord_tempo_ratio_column, rational_width);
    table.resizeColumnToContents(chord_pitched_notes_column);
    table.resizeColumnToContents(chord_unpitched_notes_column);
    table.setColumnWidth(chord_words_column, WORDS_WIDTH);
  } else if (row_type == pitched_note_type) {
    set_model(table, pitched_notes_model);

    table.setColumnWidth(pitched_note_instrument_column,
                         instrument_size.width());
    table.setColumnWidth(pitched_note_interval_column, interval_size.width());
    table.setColumnWidth(pitched_note_beats_column, rational_width);
    table.setColumnWidth(pitched_note_velocity_ratio_column, rational_width);
    table.setColumnWidth(pitched_note_words_column, WORDS_WIDTH);
  } else {
    set_model(table, unpitched_notes_model);

    table.setColumnWidth(unpitched_note_percussion_instrument_column,
                         percussion_instrument_size.width());
    table.setColumnWidth(unpitched_note_beats_column, rational_width);
    table.setColumnWidth(unpitched_note_velocity_ratio_column, rational_width);
    table.setColumnWidth(unpitched_note_words_column, WORDS_WIDTH);
  }
}

struct SwitchTable : public QTableView {
  RowType current_row_type = chord_type;

  ChordsModel chords_model;
  PitchedNotesModel pitched_notes_model;
  UnpitchedNotesModel unpitched_notes_model;
  SwitchTable(QUndoStack &undo_stack, Song &song)
      : chords_model(ChordsModel(undo_stack, song)),
        pitched_notes_model(PitchedNotesModel(undo_stack, song)),
        unpitched_notes_model(UnpitchedNotesModel(undo_stack)) {
    auto &horizontal_header = get_reference(horizontalHeader());
    auto &vertical_header = get_reference(verticalHeader());

    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSizeAdjustPolicy(SizeAdjustPolicy::AdjustToContents);

    horizontal_header.setSectionResizeMode(QHeaderView::Fixed);
    vertical_header.setSectionResizeMode(QHeaderView::Fixed);

    horizontal_header.setStretchLastSection(true);

    setMouseTracking(true);
    change_row_type(*this, chord_type, chords_model, pitched_notes_model,
                    unpitched_notes_model);
  }

  [[nodiscard]] auto sizeHint() const -> QSize override {
    const QScrollBar &vertical_scroll_bar = get_reference(verticalScrollBar());
    const QScrollBar &horizontal_scroll_bar =
        get_reference(horizontalScrollBar());
    const auto &viewport_size = viewportSizeHint();
    const auto double_frame_width = 2 * frameWidth();
    return {
        double_frame_width + viewport_size.width() +
            (vertical_scroll_bar.isVisible() ? vertical_scroll_bar.width() : 0),
        double_frame_width + viewport_size.height() +
            (horizontal_scroll_bar.isVisible() ? horizontal_scroll_bar.height()
                                               : 0)};
  }

  [[nodiscard]] auto minimumSizeHint() const -> QSize override {
    return {0, 0};
  }
};
