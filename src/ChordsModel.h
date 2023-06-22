#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT, signals
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr
#include <vector>   // for vector

#include "TreeNode.h"   // for TreeNode
#include "Utilities.h"  // for PERCENT
class QObject;          // lines 22-22

#include <qjsondocument.h>  // for QJsonDocument
#include <qjsonobject.h>    // for QJsonObject
#include <qundostack.h>     // for QUndoCommand, QUndoStack

#include <csound/csound.hpp>  // for CSOUND
#include <csound/csPerfThread.hpp>

class QByteArray;

const auto NOTE_CHORD_COLUMNS = 9;


class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  TreeNode root;
  std::vector<std::unique_ptr<const QString>>& instrument_pointers;
  QUndoStack& undo_stack;
  
  explicit ChordsModel(
    std::vector<std::unique_ptr<const QString>>& instrument_pointers_input,
    QUndoStack& undo_stack,
    QObject *parent_input = nullptr
  );

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
  void setData_directly(const QModelIndex &index, const QVariant &new_value);
  auto insertRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  void insert_children(size_t position,
                       std::vector<std::unique_ptr<TreeNode>> &insertion,
                       const QModelIndex &parent_index);
  void removeRows_internal(size_t position, size_t rows,
                           const QModelIndex &index = QModelIndex());
  auto removeRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  void remove_save(size_t position, size_t rows,
                   const QModelIndex &parent_index,
                   std::vector<std::unique_ptr<TreeNode>> &deleted_rows);

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;

  void redisplay();

  [[nodiscard]] auto verify_instruments(
      std::vector<std::unique_ptr<const QString>> &new_instrument_pointers)
      -> bool;
  [[nodiscard]] static auto verify_json(const QJsonArray &json_chords, const std::vector<std::unique_ptr<const QString>>
                              &new_instrument_pointers) -> bool;
  void load(const QJsonArray &json_chords);
  auto save() const -> QJsonArray;
};


