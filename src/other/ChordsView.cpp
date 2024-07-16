#include "justly/ChordsView.hpp"

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
#include <QString>
#include <QStyleOption>
#include <QUndoStack>
#include <QWidget>
#include <QtGlobal>
#include <memory>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "justly/ChordsModel.hpp"
#include "justly/Instrument.hpp"
#include "justly/InstrumentEditor.hpp"
#include "justly/Interval.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"
#include "other/TreeSelector.hpp"
#include "other/private.hpp"

const auto SYMBOL_WIDTH = 50;

ChordsView::ChordsView(QUndoStack *undo_stack_pointer_input, QWidget *parent)
    : QTreeView(parent),
      chords_model_pointer(new ChordsModel(undo_stack_pointer_input, this)),
      undo_stack_pointer(undo_stack_pointer_input) {
  auto *tree_selector_pointer =
      std::make_unique<TreeSelector>(chords_model_pointer).release();

  Q_ASSERT(chords_model_pointer != nullptr);
  setModel(chords_model_pointer);
  setSelectionModel(tree_selector_pointer);

  auto *factory = std::make_unique<QItemEditorFactory>().release();
  factory->registerEditor(
      qMetaTypeId<Rational>(),
      std::make_unique<QStandardItemEditorCreator<RationalEditor>>().release());
  factory->registerEditor(
      qMetaTypeId<const Instrument *>(),
      std::make_unique<QStandardItemEditorCreator<InstrumentEditor>>()
          .release());
  factory->registerEditor(
      qMetaTypeId<Interval>(),
      std::make_unique<QStandardItemEditorCreator<IntervalEditor>>().release());
  factory->registerEditor(
      qMetaTypeId<QString>(),
      std::make_unique<QStandardItemEditorCreator<QLineEdit>>().release());
  QItemEditorFactory::setDefaultFactory(factory);

  auto *header_pointer = header();

  Q_ASSERT(header_pointer != nullptr);
  header_pointer->setSectionResizeMode(QHeaderView::ResizeToContents);
  header_pointer->setSectionsMovable(false);

  setSelectionMode(QAbstractItemView::ContiguousSelection);
  setSelectionBehavior(QAbstractItemView::SelectItems);
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
  default:
    Q_ASSERT(false);
    return 0;
  }
}

auto ChordsView::viewportSizeHint() const -> QSize {
  static auto header_length = [this]() {
    auto *header_pointer = header();

    Q_ASSERT(header_pointer != nullptr);
    header_pointer->setStretchLastSection(false);
    auto temp_length = header_pointer->length();
    header_pointer->setStretchLastSection(true);

    return temp_length;
  }();

  return {header_length, QTreeView::viewportSizeHint().height()};
}

void ChordsView::remove_selected() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->removeRows(
      first_index.row(), static_cast<int>(selected_row_indexes.size()),
      first_index.parent());
}

auto ChordsView::create_editor(QModelIndex index) -> QWidget * {
  Q_ASSERT(chords_model_pointer != nullptr);

  auto *my_delegate_pointer = itemDelegate();
  Q_ASSERT(my_delegate_pointer != nullptr);

  auto *cell_editor_pointer = my_delegate_pointer->createEditor(
      viewport(), QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor_pointer != nullptr);

  my_delegate_pointer->setEditorData(cell_editor_pointer, index);

  return cell_editor_pointer;
}

void ChordsView::set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                            const QVariant &new_value) {
  auto *my_delegate_pointer = itemDelegate();

  Q_ASSERT(cell_editor_pointer != nullptr);
  const auto *cell_editor_meta_object = cell_editor_pointer->metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor_pointer->setProperty(
      cell_editor_meta_object->userProperty().name(), new_value);

  Q_ASSERT(chords_model_pointer != nullptr);
  my_delegate_pointer->setModelData(cell_editor_pointer, chords_model_pointer,
                                    index);
}

void ChordsView::copy_selected() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();
  if (selected_row_indexes.empty()) {
    auto selected_indexes = selection_model->selectedIndexes();
    Q_ASSERT(selected_indexes.size() == 1);
    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->copy_cell(selected_indexes[0]);
  } else {
    auto first_index = selected_row_indexes[0];

    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->copy_rows(first_index.row(),
                                    selected_row_indexes.size(),
                                    chords_model_pointer->parent(first_index));
  }
}

void ChordsView::paste_cell_or_after() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();

  if (selected_row_indexes.empty()) {
    
    auto selected_indexes = selection_model->selectedIndexes();
    Q_ASSERT(selected_indexes.size() == 1);
    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->paste_cell(selected_indexes[0]);
  } else {
    const auto &last_index =
        selected_row_indexes[selected_row_indexes.size() - 1];
    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->paste_rows(to_unsigned(last_index.row()) + 1,
                                    last_index.parent());
  }
}

void ChordsView::paste_into() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();

  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->paste_rows(
      to_unsigned(0),
      selected_row_indexes.empty() ? QModelIndex() : selected_row_indexes[0]);
}

void ChordsView::insert_after() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &last_index =
      selected_row_indexes[selected_row_indexes.size() - 1];
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insertRows(last_index.row() + 1, 1,
                                   last_index.parent());
}

void ChordsView::insert_into() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);
  auto selected_row_indexes = selection_model->selectedRows();
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insertRows(
      0, 1,
      selected_row_indexes.empty() ? QModelIndex() : selected_row_indexes[0]);
}