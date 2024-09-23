#pragma once

#include <QAbstractItemModel>
#include <QString>
#include <QVariant>
#include <Qt>
#include <QtGlobal>

class QObject;

[[nodiscard]] auto get_row_number(const QModelIndex &index) -> qsizetype;

struct ItemModel : public QAbstractTableModel {
  explicit ItemModel(QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override;
  [[nodiscard]] virtual auto
  get_column_name(int column_number) const -> QString = 0;
  [[nodiscard]] virtual auto
  is_column_editable(int column_number) const -> bool;

  // internal functions
  void edited_cells(qsizetype first_row_number, qsizetype number_of_rows,
                    int left_column, int right_column);

  void begin_insert_rows(qsizetype first_row_number,
                         qsizetype number_of_rows);
  void end_insert_rows();

  void begin_remove_rows(qsizetype first_row_number,
                         qsizetype number_of_rows);
  void end_remove_rows();

  void begin_reset_model();
  void end_reset_model();
};
