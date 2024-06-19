#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT

#include <memory>  // for unique_ptr

#include "justly/NoteChordField.h"  // for NoteChordField

class QModelIndex;
class QObject;
class QWidget;

struct ChordsDelegate : QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit ChordsDelegate(QObject* = nullptr);

  [[nodiscard]] auto createEditor(QWidget* parent_pointer,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
      -> QWidget* override;
};

auto create_editor(QWidget* parent_pointer, int note_chord_field)
    -> std::unique_ptr<QWidget>;