#pragma once

#include <QtWidgets/QStyledItemDelegate>

#include "rows/RowType.hpp"

struct Song;

struct SwitchDelegate : public QStyledItemDelegate {
  Song& song;
  RowType current_row_type = RowType::chord_type;

  explicit SwitchDelegate(Song& song_input, QWidget* parent)
      : QStyledItemDelegate(parent), song(song_input) {}

  auto createEditor(QWidget* parent_pointer, const QStyleOptionViewItem& option,
                    const QModelIndex& index) const -> QWidget* override;
};
