#pragma once

#include <QAbstractItemModel>
#include <QtGlobal>

class QObject;

[[nodiscard]] auto get_child_number(const QModelIndex &index) -> qsizetype;

struct ItemModel : public QAbstractTableModel {
  explicit ItemModel(QObject *parent_pointer_input = nullptr);

  // internal functions
  void edited_cells(qsizetype first_child_number, qsizetype number_of_children,
                           int left_column, int right_column);

  void begin_insert_rows(qsizetype first_child_number, qsizetype number_of_children);
  void end_insert_rows();

  void begin_remove_rows(qsizetype first_child_number, qsizetype number_of_children);
  void end_remove_rows();

  void begin_reset_model();
  void end_reset_model();
};
