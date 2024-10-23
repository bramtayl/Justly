#pragma once

#include <QAbstractItemModel>
#include <QString>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <iterator>

template <typename T> class QList;
class QObject;

template <typename Item> struct ItemsModel : public QAbstractTableModel {
  QList<Item> *items_pointer = nullptr;

  explicit ItemsModel(QList<Item> *items_pointer_input = nullptr,
                      QObject *parent_pointer_input = nullptr)
      : QAbstractTableModel(parent_pointer_input),
        items_pointer(items_pointer_input){};

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    Q_ASSERT(items_pointer != nullptr);
    return static_cast<int>(items_pointer->size());
  }

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return get_column_name(section);
    case Qt::Vertical:
      return section + 1;
    }
  };

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (is_column_editable(index.column())) {
      return uneditable | Qt::ItemIsEditable;
    }
    return uneditable;
  };

  [[nodiscard]] virtual auto get_instrument_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto get_percussion_set_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto get_percussion_instrument_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto get_interval_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto get_beats_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto get_tempo_ratio_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto get_velocity_ratio_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto get_words_column() const -> int {
    Q_ASSERT(false);
    return 0;
  };

  [[nodiscard]] virtual auto
  get_column_name(int column_number) const -> QString = 0;

  [[nodiscard]] virtual auto
  is_column_editable(int /*column_number*/) const -> bool {
    return true;
  };

  // internal functions
  void edited_cells(int first_row_number, int number_of_rows, int left_column,
                    int right_column) {
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_rows - 1, right_column),
                {Qt::DisplayRole, Qt::EditRole});
  };

  virtual void set_cells(int first_item_number, int left_column,
                         int right_column, const QList<Item> &template_items) {
    Q_ASSERT(items_pointer != nullptr);
    auto number_of_items = items_pointer->size();
    for (auto replace_number = 0; replace_number < number_of_items;
         replace_number++) {
      (*items_pointer)[first_item_number + replace_number].copy_columns_from(
          template_items.at(replace_number), left_column, right_column);
    }
    edited_cells(first_item_number, static_cast<int>(number_of_items),
                 left_column, right_column);
  }

  void begin_insert_rows(int first_row_number, int number_of_rows) {
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
  };

  void end_insert_rows() { endInsertRows(); };

  void begin_reset_model() { beginResetModel(); };

  void end_reset_model() { endResetModel(); };

  void insert_items(int first_item_number, const QList<Item> &new_items) {
    Q_ASSERT(items_pointer != nullptr);

    begin_insert_rows(first_item_number, new_items.size());
    std::copy(new_items.cbegin(), new_items.cend(),
              std::inserter(*items_pointer,
                            items_pointer->begin() + first_item_number));
    end_insert_rows();
  };

  void insert_item(int item_number, const Item &new_item) {
    Q_ASSERT(items_pointer != nullptr);

    begin_insert_rows(item_number, 1);
    items_pointer->insert(items_pointer->begin() + item_number, new_item);
    end_insert_rows();
  }

  void remove_items(int first_item_number, int number_of_items) {
    Q_ASSERT(items_pointer != nullptr);

    beginRemoveRows(QModelIndex(), first_item_number,
                    first_item_number + number_of_items - 1);
    items_pointer->erase(items_pointer->begin() + first_item_number,
                         items_pointer->begin() + first_item_number +
                             number_of_items);
    endRemoveRows();
  }
};
