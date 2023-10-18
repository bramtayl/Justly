#include "main/MyView.h"

#include <qabstractitemview.h>  // for QAbstractItemView, QAbstractIt...
#include <qheaderview.h>        // for QHeaderView, QHeaderView::Resi...
#include <qscrollbar.h>

#include "models/ChordsModel.h"
#include "utilities/SongIndex.h"  // for NoteChordField

class QWidget;

MyView::MyView(QWidget* parent) : QTreeView(parent) {
  header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
}

auto MyView::sizeHint() const -> QSize {
  header()->setStretchLastSection(false);
  auto header_length = header()->length();
  header()->setStretchLastSection(true);

  return {
      header_length + lineWidth() * 2 + verticalScrollBar()->sizeHint().width(),
      QAbstractItemView::sizeHint().height()};
}

auto MyView::sizeHintForColumn(int column) const -> int {
  return ChordsModel::get_cell_size(static_cast<NoteChordField>(column))
      .width();
}
