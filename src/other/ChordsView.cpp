#include "other/ChordsView.hpp"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMetaType>
#include <QtGlobal>
#include <memory>

#include "cell_editors/InstrumentEditor.hpp"
#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "cell_values/Instrument.hpp"
#include "cell_values/Interval.hpp"
#include "cell_values/Rational.hpp"
#include "models/ChordsModel.hpp"
#include "other/TreeSelector.hpp"

class QString;
class QUndoStack;

static const auto DEFAULT_VIEW_WIDTH = 750;

ChordsView::ChordsView(QUndoStack *undo_stack_pointer_input, QWidget *parent)
    : QTreeView(parent),
      chords_model_pointer(new ChordsModel(undo_stack_pointer_input, this)) {
  auto *tree_selector_pointer =
      std::make_unique<TreeSelector>(chords_model_pointer).release();

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
  Q_ASSERT(header_pointer != nullptr);
  header_pointer->setSectionResizeMode(QHeaderView::ResizeToContents);

  setMouseTracking(true);
}

auto ChordsView::viewportSizeHint() const -> QSize {
  return {DEFAULT_VIEW_WIDTH, QTreeView::viewportSizeHint().height()};
}
