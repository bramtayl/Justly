#include "other/ChordsView.hpp"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMetaObject>
#include <QMetaType>
#include <QStyleOption>
#include <QWidget>
#include <QtGlobal>
#include <memory>

#include "cell_editors/InstrumentEditor.hpp"
#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/Rational.hpp"
#include "models/ChordsModel.hpp"
#include "other/TreeSelector.hpp"
#include "other/conversions.hpp"

class QString;
class QUndoStack;

const auto DEFAULT_VIEW_WIDTH = 750;

ChordsView::ChordsView(QUndoStack *undo_stack_pointer_input, QWidget *parent)
    : QTreeView(parent),
      chords_model_pointer(new ChordsModel(undo_stack_pointer_input, this)) {
  auto *tree_selector_pointer =
      std::make_unique<TreeSelector>(chords_model_pointer).release();

  Q_ASSERT(chords_model_pointer != nullptr);
  setModel(chords_model_pointer);
  setSelectionModel(tree_selector_pointer);

  auto *factory_pointer = std::make_unique<QItemEditorFactory>().release();
  factory_pointer->registerEditor(
      qMetaTypeId<Rational>(),
      std::make_unique<QStandardItemEditorCreator<RationalEditor>>().release());
  factory_pointer->registerEditor(
      qMetaTypeId<const Instrument *>(),
      std::make_unique<QStandardItemEditorCreator<InstrumentEditor>>()
          .release());
  factory_pointer->registerEditor(
      qMetaTypeId<Interval>(),
      std::make_unique<QStandardItemEditorCreator<IntervalEditor>>().release());
  factory_pointer->registerEditor(
      qMetaTypeId<QString>(),
      std::make_unique<QStandardItemEditorCreator<QLineEdit>>().release());
  QItemEditorFactory::setDefaultFactory(factory_pointer);

  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);

  auto *header_pointer = header();
  header_pointer->setSectionResizeMode(QHeaderView::ResizeToContents);
}

auto ChordsView::viewportSizeHint() const -> QSize {
  return {DEFAULT_VIEW_WIDTH, QTreeView::viewportSizeHint().height()};
}

auto ChordsView::get_chord_index(size_t chord_number,
                                 NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return chords_model_pointer->get_chord_index(chord_number, note_chord_column);
}

auto ChordsView::get_note_index(size_t chord_number, size_t note_number,
                                NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return chords_model_pointer->get_note_index(chord_number, note_number,
                                              note_chord_column);
}

void ChordsView::delete_selected() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->delete_selected(selection_model_pointer->selection());
}

auto ChordsView::create_editor(QModelIndex index) -> QWidget * {
  Q_ASSERT(chords_model_pointer != nullptr);

  auto *delegate_pointer = itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  auto *cell_editor_pointer =
      delegate_pointer->createEditor(viewport(), QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor_pointer != nullptr);

  delegate_pointer->setEditorData(cell_editor_pointer, index);

  return cell_editor_pointer;
}

void ChordsView::set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                            const QVariant &new_value) {
  Q_ASSERT(cell_editor_pointer != nullptr);
  const auto *cell_editor_meta_object = cell_editor_pointer->metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor_pointer->setProperty(
      cell_editor_meta_object->userProperty().name(), new_value);

  auto *delegate_pointer = itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);
  Q_ASSERT(chords_model_pointer != nullptr);
  delegate_pointer->setModelData(cell_editor_pointer, chords_model_pointer,
                                 index);
}

void ChordsView::cut_selected() {
  copy_selected();
  delete_selected();
}

void ChordsView::copy_selected() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->copy_selected(selection_model_pointer->selection());
}

void ChordsView::paste_cells_or_after() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);

  auto selected_row_indexes = selection_model_pointer->selectedRows();
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->paste_cells_or_after(
      selection_model_pointer->selection());
}

void ChordsView::paste_into() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);

  auto selected_row_indexes = selection_model_pointer->selectedRows();

  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->paste_rows(to_size_t(0), selected_row_indexes.empty()
                                                     ? QModelIndex()
                                                     : selected_row_indexes[0]);
}

void ChordsView::insert_after() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);

  auto selected_row_indexes = selection_model_pointer->selectedRows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &last_index =
      selected_row_indexes[selected_row_indexes.size() - 1];
  Q_ASSERT(chords_model_pointer != nullptr);
  auto inserted = chords_model_pointer->insertRows(last_index.row() + 1, 1,
                                                   last_index.parent());
  Q_ASSERT(inserted);
}

void ChordsView::insert_into() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  auto selected_row_indexes = selection_model_pointer->selectedRows();
  Q_ASSERT(chords_model_pointer != nullptr);
  auto inserted = chords_model_pointer->insertRows(
      0, 1,
      selected_row_indexes.empty() ? QModelIndex() : selected_row_indexes[0]);
  Q_ASSERT(inserted);
}

void ChordsView::expand_selected() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  for (const auto& index : selection_model_pointer->selectedRows()) {
    expand(index);
  };
}

void ChordsView::collapse_selected() {
  auto *selection_model_pointer = selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  for (const auto& index : selection_model_pointer->selectedRows()) {
    collapse(index);
  };
}