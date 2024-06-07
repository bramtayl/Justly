#include "src/ChordsDelegate.h"

#include <qabstractitemmodel.h>   // for QModelIndex
#include <qlineedit.h>            // for QLineEdit
#include <qspinbox.h>             // for QDoubleSpinBox, QSpinBox
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qwidget.h>              // for QWidget

#include <memory>  // for make_unique, unique_ptr

#include "justly/NoteChord.h"      // for MAXIMUM_VOLUME_PERCENT, MINIMUM...
#include "justly/InstrumentEditor.h"  // for InstrumentEditor
#include "src/IntervalEditor.h"    // for IntervalEditor

ChordsDelegate::ChordsDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto ChordsDelegate::create_editor(QWidget *parent_pointer,
                                   NoteChordField note_chord_field)
    -> std::unique_ptr<QWidget> {
  switch (static_cast<NoteChordField>(note_chord_field)) {
    case instrument_column:
      return std::make_unique<InstrumentEditor>(parent_pointer);
    case interval_column:
      return std::make_unique<IntervalEditor>(parent_pointer);
    case beats_column: {
      auto spin_box_pointer = std::make_unique<QSpinBox>(parent_pointer);
      spin_box_pointer->setMinimum(MINIMUM_BEATS);
      spin_box_pointer->setMaximum(MAXIMUM_BEATS);
      return spin_box_pointer;
    }
    case volume_percent_column: {
      auto spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer);
      spin_box_pointer->setDecimals(1);
      spin_box_pointer->setMinimum(MINIMUM_VOLUME_PERCENT);
      spin_box_pointer->setMaximum(MAXIMUM_VOLUME_PERCENT);
      spin_box_pointer->setSuffix("%");
      return spin_box_pointer;
    }
    case tempo_percent_column: {
      auto spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer);
      spin_box_pointer->setDecimals(1);
      spin_box_pointer->setMinimum(MINIMUM_VOLUME_PERCENT);
      spin_box_pointer->setMaximum(MAXIMUM_VOLUME_PERCENT);
      spin_box_pointer->setSuffix("%");
      return spin_box_pointer;
    }
    case words_column: {
      return std::make_unique<QLineEdit>(parent_pointer);
    }
    case symbol_column: {
      return {};
    }
    default:
      return {};
  }
}

auto ChordsDelegate::createEditor(QWidget *parent_pointer,
                                  const QStyleOptionViewItem & /*option*/,
                                  const QModelIndex &index) const -> QWidget * {
  return create_editor(parent_pointer,
                       static_cast<NoteChordField>(index.column()))
      .release();
}
