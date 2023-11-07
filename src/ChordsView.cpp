#include "src/ChordsView.h"

#include <qabstractitemview.h>  // for QAbstractItemView, QAbstractIt...
#include <qheaderview.h>        // for QHeaderView, QHeaderView::Resi...
#include <qscrollbar.h>
#include <qwidget.h>

#include <memory>

#include "justly/utilities/SongIndex.h"  // for NoteChordField
#include "src/ChordsDelegate.h"

const auto SYMBOL_WIDTH = 50;

ChordsView::ChordsView(QWidget* parent) : QTreeView(parent) {
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  header()->setSectionsMovable(false);
  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  setItemDelegate(std::make_unique<ChordsDelegate>(this).release());
}

auto ChordsView::viewportSizeHint() const -> QSize {
  header()->setStretchLastSection(false);
  auto header_length = header()->length();
  header()->setStretchLastSection(true);

  return {header_length, QTreeView::viewportSizeHint().height()};
}

// make sure we save room for the editor
auto ChordsView::sizeHintForColumn(int column) const -> int {
  auto note_chord_field = static_cast<NoteChordField>(column);
  // cache column widths
  // no editor for the symbol column
  static auto instrument_width =
      ChordsDelegate::create_editor(nullptr, instrument_column)
          ->sizeHint()
          .width();
  static auto interval_width =
      ChordsDelegate::create_editor(nullptr, interval_column)
          ->sizeHint()
          .width();
  static auto beats_width =
      ChordsDelegate::create_editor(nullptr, beats_column)->sizeHint().width();
  static auto volume_percent_width =
      ChordsDelegate::create_editor(nullptr, volume_percent_column)
          ->sizeHint()
          .width();
  static auto tempo_percent_width =
      ChordsDelegate::create_editor(nullptr, tempo_percent_column)
          ->sizeHint()
          .width();
  static auto words_width =
      ChordsDelegate::create_editor(nullptr, words_column)->sizeHint().width();
  switch (note_chord_field) {
    case symbol_column:
      return SYMBOL_WIDTH;
    case instrument_column:
      return instrument_width;
    case interval_column:
      return interval_width;
    case beats_column:
      return beats_width;
    case volume_percent_column:
      return volume_percent_width;
    case tempo_percent_column:
      return tempo_percent_width;
    default:
      return words_width;
  }
}
