#pragma once

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTableView>

#include "cell_editors/IntervalEditor.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "other/helpers.hpp"
#include "widgets/SwitchDelegate.hpp"

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
  UndoRowsModel<PitchedVoice> pitched_voices_model;
  UndoRowsModel<UnpitchedVoice> unpitched_voices_model;
  SwitchDelegate &delegate;
  SwitchTable(QUndoStack &undo_stack, Song &song)
      : chords_model(ChordsModel(undo_stack, song)),
        pitched_notes_model(PitchedNotesModel(undo_stack, song)),
        unpitched_notes_model(UnpitchedNotesModel(undo_stack, song)),
        pitched_voices_model(UndoRowsModel<PitchedVoice>(undo_stack, song)),
        unpitched_voices_model(UndoRowsModel<UnpitchedVoice>(undo_stack, song)),
        delegate(get_reference(
            new SwitchDelegate( // NOLINT(cppcoreguidelines-owning-memory)
                song, this))) {
    chords_model.set_rows_pointer(&song.chords);
    pitched_voices_model.set_rows_pointer(&song.pitched_voices);
    unpitched_voices_model.set_rows_pointer(&song.unpitched_voices);

    setItemDelegate(&delegate);
    auto &horizontal_header = get_reference(horizontalHeader());
    auto &vertical_header = get_reference(verticalHeader());

    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSizeAdjustPolicy(SizeAdjustPolicy::AdjustToContents);

    horizontal_header.setSectionResizeMode(QHeaderView::Fixed);
    horizontal_header.setStretchLastSection(true);
    vertical_header.setSectionResizeMode(QHeaderView::Fixed);
    vertical_header.setDefaultSectionSize(get_minimum_size<IntervalEditor>().height());

    setMouseTracking(true);
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
