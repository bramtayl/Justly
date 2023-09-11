#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qjsonarray.h>
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qstring.h>
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"
#include "StableIndex.h"  // for StableIndex

class Editor;
class QObject;  // lines 22-22
class TreeNode;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  TreeNode &root;
  Editor &editor;

  explicit ChordsModel(TreeNode &root,
                       Editor &editor_input,
                       QObject *parent_pointer_input = nullptr);
  void begin_reset_model();
  void end_reset_model();

  [[nodiscard]] auto get_node(const QModelIndex &index) -> TreeNode &;
  [[nodiscard]] auto const_node_from_index(const QModelIndex &index) const
      -> const TreeNode &;
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;
  [[nodiscard]] static auto column_flags(int column) -> Qt::ItemFlags;
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
  auto copy(int first_index, int number_of_children, const QModelIndex &parent_index,
            std::vector<std::unique_ptr<TreeNode>> &copy_to) -> int;
  void directly_set_data(const QModelIndex &index,
                            const QVariant &new_value);
  auto insertRows(int first_index, int number_of_children,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  void insert_children(int first_index,
                       std::vector<std::unique_ptr<TreeNode>> &insertion,
                       const QModelIndex &parent_index);
  auto removeRows(int first_index, int number_of_children,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  void remove_save(int first_index, int number_of_children, const QModelIndex &parent_index,
                   std::vector<std::unique_ptr<TreeNode>> &deleted_children);

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;

  [[nodiscard]] auto get_stable_index(const QModelIndex &index) const
      -> StableIndex;
  [[nodiscard]] auto get_unstable_index(const StableIndex &index) const
      -> QModelIndex;
  [[nodiscard]] auto get_level(const QModelIndex &index) -> TreeLevel;
  void insert_json_children(int first_index, const QJsonArray& insertion, const QModelIndex &parent_index);
  auto copy_json(int first_index, int number_of_children, const QModelIndex &parent_index) -> QJsonArray;
  [[nodiscard]] auto verify_json_children(const QString& paste_text, const QModelIndex &parent_index) const -> bool;
};
