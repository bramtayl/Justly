#include "editors/ChordsDelegate.hpp"

#include <qabstractitemmodel.h>   // for QModelIndex
#include <qlineedit.h>            // for QLineEdit
#include <qspinbox.h>             // for QDoubleSpinBox, QSpinBox
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qwidget.h>              // for QWidget

#include <memory>  // for make_unique, unique_ptr

#include "justly/InstrumentEditor.hpp"  // for InstrumentEditor
#include "justly/IntervalEditor.hpp"    // for IntervalEditor
#include "justly/NoteChord.hpp"         // for MAX_VOLUME_PERCENT, MINIMUM...
#include "justly/NoteChordField.hpp"    // for beats_column, instrument_column

ChordsDelegate::ChordsDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto ChordsDelegate::createEditor(QWidget *parent_pointer,
                                  const QStyleOptionViewItem & /*option*/,
                                  const QModelIndex &index) const -> QWidget * {
  return create_editor(parent_pointer, index.column()).release();
}

auto create_editor(QWidget *parent_pointer, int note_chord_field)
    -> std::unique_ptr<QWidget> {
  switch (note_chord_field) {
    case instrument_column:
      return std::make_unique<InstrumentEditor>(parent_pointer);
    case interval_column:
      return std::make_unique<IntervalEditor>(parent_pointer);
    case beats_column: {
      auto spin_box_pointer = std::make_unique<QSpinBox>(parent_pointer);
      spin_box_pointer->setMinimum(MIN_BEATS);
      spin_box_pointer->setMaximum(MAX_BEATS);
      return spin_box_pointer;
    }
    case volume_percent_column: {
      auto spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer);
      spin_box_pointer->setDecimals(1);
      spin_box_pointer->setMinimum(MIN_VOLUME_PERCENT);
      spin_box_pointer->setMaximum(MAX_VOLUME_PERCENT);
      spin_box_pointer->setSuffix("%");
      return spin_box_pointer;
    }
    case tempo_percent_column: {
      auto spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer);
      spin_box_pointer->setDecimals(1);
      spin_box_pointer->setMinimum(MIN_TEMPO_PERCENT);
      spin_box_pointer->setMaximum(MAX_TEMPO_PERCENT);
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