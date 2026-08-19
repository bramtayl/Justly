#include "widgets/SwitchTable.hpp"

SwitchTable::SwitchTable(QUndoStack &undo_stack, Song &song)
    : chords_model(ChordsModel(undo_stack, song)),
      pitched_notes_model(PitchedNotesModel(undo_stack, song)),
      unpitched_notes_model(UnpitchedNotesModel(undo_stack, song)),
      pitched_voices_model(
          PitchedVoicesModel(get_reference(this), undo_stack, song)),
      unpitched_voices_model(
          UnpitchedVoicesModel(get_reference(this), undo_stack, song)),
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
  vertical_header.setDefaultSectionSize(
      get_minimum_size<IntervalEditor>().height());

  setMouseTracking(true);
}

auto SwitchTable::sizeHint() const -> QSize {
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

auto SwitchTable::minimumSizeHint() const -> QSize {
  return {0, 0};
}
