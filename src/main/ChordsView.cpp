#include "src/main/ChordsView.h"

#include <qabstractitemview.h>  // for QAbstractItemView, QAbstractIt...
#include <qheaderview.h>        // for QHeaderView, QHeaderView::Resi...
#include <qscrollbar.h>
#include <qwidget.h>

#include <memory>

#include "justly/utilities/SongIndex.h"  // for NoteChordField
#include "src/main/ChordsDelegate.h"

ChordsView::ChordsView(QWidget* parent) : QTreeView(parent) {
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setItemDelegate(std::make_unique<ChordsDelegate>(this).release());
}

auto ChordsView::sizeHint() const -> QSize {
  header()->setStretchLastSection(false);
  auto header_length = header()->length();
  header()->setStretchLastSection(true);

  return {
      header_length + lineWidth() * 2 + verticalScrollBar()->sizeHint().width(),
      QAbstractItemView::sizeHint().height()};
}

auto ChordsView::sizeHintForColumn(int column) const -> int {
  auto note_chord_field = static_cast<NoteChordField>(column);
  switch (note_chord_field) {
    case symbol_column:
      return QTreeView::sizeHintForColumn(column);
    default:
      return ChordsDelegate::create_editor(nullptr, note_chord_field)
          ->sizeHint()
          .width();
  }
}
