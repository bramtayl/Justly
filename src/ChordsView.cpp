#include "src/ChordsView.h"

#include <qabstractitemview.h>    // for QAbstractItemView, QAbstractItemV...
#include <qabstractscrollarea.h>  // for QAbstractScrollArea, QAbstractScr...
#include <qheaderview.h>          // for QHeaderView, QHeaderView::ResizeT...
#include <qwidget.h>              // for QWidget

#include <memory>  // for make_unique, __unique_ptr_t, uniq...

#include "justly/NoteChordField.h"  // for NoteChordField, symbol_column
#include "src/ChordsDelegate.h"     // for ChordsDelegate

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
