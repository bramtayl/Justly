#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT, signals
#include <qvariant.h>            // for QVariant
#include <stddef.h>              // for size_t

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "TreeNode.h"  // for TreeNode
class QObject;         // lines 13-13

const int DEFAULT_FREQUENCY = 220;
const int DEFAULT_VOLUME_PERCENT = 50;
const int DEFAULT_TEMPO = 200;
const auto MIN_FREQUENCY = 60;
const auto MAX_FREQUENCY = 440;
const auto MIN_VOLUME_PERCENT = 0;
const auto MAX_VOLUME_PERCENT = 100;
const auto MIN_TEMPO = 100;
const auto MAX_TEMPO = 800;

const int NOTE_CHORD_COLUMNS = 9;

class Song : public QAbstractItemModel {
  Q_OBJECT

 public:
  const std::set<QString>& instruments;
  int frequency = DEFAULT_FREQUENCY;
  int volume_percent = DEFAULT_VOLUME_PERCENT;
  int tempo = DEFAULT_TEMPO;

  // pointer so the pointer, but not object, can be constant
  TreeNode root;

  explicit Song(const std::set<QString>& instruments, QObject *parent = nullptr);

  [[nodiscard]] auto node_from_index(const QModelIndex &index) -> TreeNode &;
  [[nodiscard]] auto const_node_from_index(const QModelIndex &index) const
      -> const TreeNode &;
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;
  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const
      -> QVariant override;
  [[nodiscard]] auto index(int row, int column,
                           const QModelIndex &parent = QModelIndex()) const
      -> QModelIndex override;
  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;
  [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const
      -> int override;
  [[nodiscard]] auto columnCount(
      const QModelIndex &parent = QModelIndex()) const -> int override;
  auto setData_directly(const QModelIndex &index, const QVariant &new_value,
                        int role) -> bool;
  auto insertRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  auto insert_children(size_t position,
                       std::vector<std::unique_ptr<TreeNode>> &insertion,
                       const QModelIndex &parent_index) -> void;
  auto removeRows_internal(size_t position, size_t rows,
                           const QModelIndex &index = QModelIndex()) -> void;
  auto removeRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  auto remove_save(size_t position, size_t rows,
                   const QModelIndex &parent_index,
                   std::vector<std::unique_ptr<TreeNode>> &deleted_rows)
      -> void;

  void save(const QString &file) const;
  auto setData(const QModelIndex &index, const QVariant &new_value, int role)
      -> bool override;

 signals:
  void set_data_signal(const QModelIndex &index, const QVariant &new_value,
                       int role);
};
