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
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
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
  switch (note_chord_field) {
    // no editor for the symbol column
    case symbol_column:
      return SYMBOL_WIDTH;
    default:
      return ChordsDelegate::create_editor(nullptr, note_chord_field)
          ->sizeHint()
          .width();
  }
}
