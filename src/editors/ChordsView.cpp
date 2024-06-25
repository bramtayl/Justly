#include "editors/ChordsView.hpp"

#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemV...
#include <qabstractscrollarea.h>  // for QAbstractScrollArea, QAbstractScr...
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeT...
#include <qlineedit.h>
#include <qwidget.h>  // for QWidget

#include <memory>  // for make_unique, __unique_ptr_t, uniq...

#include "editors/InstrumentEditor.hpp"
#include "editors/IntervalEditor.hpp"
#include "editors/RationalEditor.hpp"
#include "editors/sizes.hpp"
#include "justly/NoteChordField.hpp"  // for NoteChordField, symbol_column
#include "models/ChordsModel.hpp"

const auto SYMBOL_WIDTH = 50;

ChordsView::ChordsView(QWidget* parent) : QTreeView(parent) {
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  header()->setSectionsMovable(false);
  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
}

// make sure we save room for the editor
auto ChordsView::sizeHintForColumn(int column) const -> int {
  static auto INSTRUMENT_WIDTH =
      get_instrument_size().width();
  static auto INTERVAL_WIDTH =
      get_interval_size().width();
  static auto RATIONAL_WIDTH =
      get_rational_size().width();
  static auto WORDS_WIDTH = get_words_size().width();

  switch (column) {
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
