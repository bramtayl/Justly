#include "editors/ChordsView.hpp"

#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemV...
#include <qabstractscrollarea.h>  // for QAbstractScrollArea, QAbstractScr...
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeT...
#include <qwidget.h>              // for QWidget

#include <memory>  // for make_unique, __unique_ptr_t, uniq...

#include "editors/ChordsDelegate.hpp"  // for ChordsDelegate
#include "justly/NoteChordField.hpp"   // for NoteChordField, symbol_column

const auto SYMBOL_WIDTH = 50;

ChordsView::ChordsView(QWidget* parent) : QTreeView(parent) {
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  header()->setSectionsMovable(false);
  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
  setItemDelegate(std::make_unique<ChordsDelegate>(this).release());
}

// make sure we save room for the editor
auto ChordsView::sizeHintForColumn(int column) const -> int {
  static auto INSTRUMENT_WIDTH =
      create_editor(nullptr, instrument_column)->sizeHint().width();
  static auto INTERVAL_WIDTH =
      create_editor(nullptr, interval_column)->sizeHint().width();
  static auto BEATS_WIDTH =
      create_editor(nullptr, beats_column)->sizeHint().width();
  static auto VOLUME_PERCENT_WIDTH =
      create_editor(nullptr, volume_ratio_column)->sizeHint().width();
  static auto TEMPO_PERCENT_WIDTH =
      create_editor(nullptr, tempo_ratio_column)->sizeHint().width();
  static auto WORDS_WIDTH =
      create_editor(nullptr, words_column)->sizeHint().width();

  switch (column) {
    case symbol_column:
      return SYMBOL_WIDTH;
    case instrument_column:
      return INSTRUMENT_WIDTH;
    case interval_column:
      return INTERVAL_WIDTH;
    case beats_column:
      return BEATS_WIDTH;
    case volume_ratio_column:
      return VOLUME_PERCENT_WIDTH;
    case tempo_ratio_column:
      return TEMPO_PERCENT_WIDTH;
    case words_column:
      return WORDS_WIDTH;
    default:
      return 0;
  }
}

auto ChordsView::viewportSizeHint() const -> QSize {
  static auto header_length = [this]() {
    header()->setStretchLastSection(false);
    auto temp_length = header()->length();
    header()->setStretchLastSection(true);
    return temp_length;
  }();

  return {header_length, QTreeView::viewportSizeHint().height()};
}
