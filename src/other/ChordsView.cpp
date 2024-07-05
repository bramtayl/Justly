#include "justly/ChordsView.hpp"

#include <qabstractitemdelegate.h>  // for QAbstractItemDelegate
#include <qabstractitemmodel.h>     // for QModelIndex
#include <qabstractitemview.h>      // for QAbstractItemView, QAbs...
#include <qabstractscrollarea.h>    // for QAbstractScrollArea
#include <qassert.h>                // for Q_ASSERT
#include <qheaderview.h>            // for QHeaderView, QHeaderVie...
#include <qitemeditorfactory.h>     // for QStandardItemEditorCreator
#include <qitemselectionmodel.h>    // for QItemSelectionModel
#include <qlineedit.h>              // for QLineEdit
#include <qlist.h>                  // for QList
#include <qmetaobject.h>            // for QMetaProperty
#include <qmetatype.h>              // for qMetaTypeId
#include <qobjectdefs.h>            // for QMetaObject
#include <qstring.h>
#include <qstyleoption.h>           // for QStyleOptionViewItem
#include <qundostack.h>
#include <qwidget.h>                // for QWidget

#include <memory>                 // for make_unique, __unique_p...
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "cell_editors/IntervalEditor.hpp"  // for IntervalEditor
#include "cell_editors/RationalEditor.hpp"  // for RationalEditor
#include "cell_editors/sizes.hpp"           // for get_instrument_size
#include "justly/ChordsModel.hpp"           // for ChordsModel, to_parent_...
#include "justly/Instrument.hpp"            // for Instrument
#include "justly/InstrumentEditor.hpp"      // for InstrumentEditor
#include "justly/Interval.hpp"              // for Interval
#include "justly/NoteChordField.hpp"        // for to_note_chord_field
#include "justly/Rational.hpp"              // for Rational
#include "other/TreeSelector.hpp"           // for TreeSelector

const auto SYMBOL_WIDTH = 50;

ChordsView::ChordsView(QUndoStack *undo_stack_pointer_input, QWidget *parent)
    : QTreeView(parent),
      chords_model_pointer(
          std::make_unique<ChordsModel>(undo_stack_pointer_input, this)
              .release()),
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
  auto selected_row_indexes = selectionModel()->selectedRows();
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
  nlohmann::json copied;
  if (selected_row_indexes.empty()) {
    Q_ASSERT(selectionModel() != nullptr);
    auto selected_indexes = selection_model->selectedIndexes();
    Q_ASSERT(selected_indexes.size() == 1);
    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->copy_cell(selected_indexes[0]);
  } else {
    auto first_index = selected_row_indexes[0];

    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->copy_rows(
        first_index.row(), selected_row_indexes.size(),
        to_parent_number(chords_model_pointer->parent(first_index)));
  }
}

void ChordsView::paste_cell() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);
  auto selected_indexes = selection_model->selectedIndexes();
  Q_ASSERT(selected_indexes.size() == 1);
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->paste_cell(selected_indexes[0]);
}

void ChordsView::paste_before() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];

  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->paste_rows(first_index.row(), first_index.parent());
}

void ChordsView::paste_after() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &last_index =
      selected_row_indexes[selected_row_indexes.size() - 1];

  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->paste_rows(last_index.row() + 1, last_index.parent());
}

void ChordsView::paste_into() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();

  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->paste_rows(0, selected_row_indexes.empty()
                                          ? QModelIndex()
                                          : selected_row_indexes[0]);
}

void ChordsView::insert_before() {
  auto *selection_model = selectionModel();
  Q_ASSERT(selection_model != nullptr);

  auto selected_row_indexes = selection_model->selectedRows();
  Q_ASSERT(!selected_row_indexes.empty());
  const auto &first_index = selected_row_indexes[0];
  chords_model_pointer->insertRows(first_index.row(), 1, first_index.parent());
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