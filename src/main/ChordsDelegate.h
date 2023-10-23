#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT

#include <memory>

#include "justly/utilities/SongIndex.h"

class QModelIndex;
class QObject;
class QWidget;

class ChordsDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit ChordsDelegate(QObject* = nullptr);

  [[nodiscard]] auto createEditor(QWidget* parent_pointer,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
      -> QWidget* override;
  void createColumnEditor(int column);
  static auto create_editor(QWidget* parent_pointer,
                            NoteChordField note_chord_field)
      -> std::unique_ptr<QWidget>;
};
