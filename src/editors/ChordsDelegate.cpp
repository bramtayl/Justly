#include "editors/ChordsDelegate.hpp"

#include <qabstractitemmodel.h>   // for QModelIndex
#include <qlineedit.h>            // for QLineEdit
#include <qspinbox.h>             // for QDoubleSpinBox, QSpinBox
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qwidget.h>              // for QWidget

#include <memory>  // for make_unique, unique_ptr

#include "editors/InstrumentEditor.hpp"  // for InstrumentEditor
#include "editors/IntervalEditor.hpp"    // for IntervalEditor
#include "editors/RationalEditor.hpp"
#include "justly/NoteChord.hpp"       // for MAX_VOLUME_PERCENT, MINIMUM...
#include "justly/NoteChordField.hpp"  // for beats_column, instrument_column

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
    case instrument_column: {
      return std::make_unique<InstrumentEditor>(parent_pointer);
    }
    case beats_column: {
      return std::make_unique<RationalEditor>(parent_pointer);
    }
    case interval_column: {
      return std::make_unique<IntervalEditor>(parent_pointer);
    }
    case volume_ratio_column: {
      return std::make_unique<RationalEditor>(parent_pointer);
    }
    case words_column: {
      return std::make_unique<QLineEdit>(parent_pointer);
    }
    case tempo_ratio_column: {
      return std::make_unique<RationalEditor>(parent_pointer);
    }
    case symbol_column: {
      return {};
    }
    default:
      return {};
  }
}
