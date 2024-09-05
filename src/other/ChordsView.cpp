#include "other/ChordsView.hpp"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMetaType>
#include <QSpinBox>
#include <QtGlobal>
#include <memory>

#include "chord/ChordsModel.hpp"
#include "instrument/Instrument.hpp"
#include "instrument/InstrumentEditor.hpp"
#include "interval/Interval.hpp"
#include "interval/IntervalEditor.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_instrument/PercussionInstrumentEditor.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "percussion_set/PercussionSetEditor.hpp"
#include "rational/Rational.hpp"
#include "rational/RationalEditor.hpp"

class QString;
class QUndoStack;

static const auto DEFAULT_VIEW_WIDTH = 750;

ChordsView::ChordsView(QUndoStack *undo_stack_pointer_input, QWidget *parent)
    : QTreeView(parent),
      chords_model_pointer(new ChordsModel(undo_stack_pointer_input, this)) {

  auto *factory_pointer = std::make_unique<QItemEditorFactory>().release();
  factory_pointer->registerEditor(
      qMetaTypeId<Rational>(),
      std::make_unique<QStandardItemEditorCreator<RationalEditor>>().release());
  factory_pointer->registerEditor(
      qMetaTypeId<const PercussionInstrument *>(),
      std::make_unique<QStandardItemEditorCreator<PercussionInstrumentEditor>>()
          .release());
  factory_pointer->registerEditor(
      qMetaTypeId<const PercussionSet *>(),
      std::make_unique<QStandardItemEditorCreator<PercussionSetEditor>>()
          .release());
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
  factory_pointer->registerEditor(
      qMetaTypeId<int>(),
      std::make_unique<QStandardItemEditorCreator<QSpinBox>>().release());
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
