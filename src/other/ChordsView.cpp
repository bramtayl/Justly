#include "other/ChordsView.hpp"

#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractIte...
#include <qabstractscrollarea.h>  // for QAbstractScrollArea, QAbstractS...
#include <qassert.h>              // for Q_ASSERT
#include <qheaderview.h>          // for QHeaderView, QHeaderView::Resiz...

#include "cell_editors/sizes.hpp"     // for get_instrument_size, get_interv...
#include "justly/NoteChordField.hpp"  // for beats_column, instrument_column

const auto SYMBOL_WIDTH = 50;

ChordsView::ChordsView(QWidget* parent) : QTreeView(parent) {
  auto* header_pointer = header();

  Q_ASSERT(header_pointer != nullptr);
  header_pointer->setSectionResizeMode(QHeaderView::ResizeToContents);
  header_pointer->setSectionsMovable(false);

  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
}

// make sure we save room for the editor
auto ChordsView::sizeHintForColumn(int column) const -> int {
  static auto INSTRUMENT_WIDTH = get_instrument_size().width();
  static auto INTERVAL_WIDTH = get_interval_size().width();
  static auto RATIONAL_WIDTH = get_rational_size().width();
  static auto WORDS_WIDTH = get_words_size().width();

  switch (to_note_chord_field(column)) {
    case symbol_column:
      return SYMBOL_WIDTH;
    case instrument_column:
      return INSTRUMENT_WIDTH;
    case beats_column:
      return RATIONAL_WIDTH;
    case interval_column:
      return INTERVAL_WIDTH;
    case volume_ratio_column:
      return RATIONAL_WIDTH;
    case words_column:
      return WORDS_WIDTH;
    case tempo_ratio_column:
      return RATIONAL_WIDTH;
  }
}

auto ChordsView::viewportSizeHint() const -> QSize {
  static auto header_length = [this]() {
    auto* header_pointer = header();

    Q_ASSERT(header_pointer != nullptr);
    header_pointer->setStretchLastSection(false);
    auto temp_length = header_pointer->length();
    header_pointer->setStretchLastSection(true);

    return temp_length;
  }();

  return {header_length, QTreeView::viewportSizeHint().height()};
}
