#pragma once

#include "items_model/SetCell.hpp"
#include <QAbstractItemModel>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <iterator>

template <typename T> class QList;
class QObject;
class QUndoCommand;
class QUndoStack;

template <typename Item> struct ItemsModel : public QAbstractTableModel {
  QList<Item> *items_pointer = nullptr;
  QUndoStack *const undo_stack_pointer;

  explicit ItemsModel(QUndoStack *undo_stack_pointer_input,
                      QList<Item> *items_pointer_input = nullptr,
                      QObject *parent_pointer_input = nullptr)
      : QAbstractTableModel(parent_pointer_input),
        items_pointer(items_pointer_input),
        undo_stack_pointer(undo_stack_pointer_input){};

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

  [[nodiscard]] virtual auto
  get_column_name(int column_number) const -> QString = 0;

  [[nodiscard]] virtual auto
  is_column_editable(int /*column_number*/) const -> bool {
    return true;
  };

  [[nodiscard]] virtual auto get_status(int row_number) const -> QString = 0;

  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override {
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
      return {};
    }

    Q_ASSERT(index.isValid());
    auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      return get_status(row_number);
    }

    Q_ASSERT(items_pointer != nullptr);
    return items_pointer->at(row_number).get_data(index.column());
  }

  void set_data_directly(const QModelIndex &index, const QVariant &new_value) {
    Q_ASSERT(items_pointer != nullptr);
    auto &item = (*items_pointer)[index.row()];
    item.set_data_directly(index.column(), new_value);
  }

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override {
    // only set data for edit
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack_pointer->push(
        new SetCell<Item>( // NOLINT(cppcoreguidelines-owning-memory)
            this, index, new_value));
    return true;
  };

  auto get_item(int item_number) -> const Item & {
    Q_ASSERT(items_pointer != nullptr);
    return items_pointer->at(item_number);
  }

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
