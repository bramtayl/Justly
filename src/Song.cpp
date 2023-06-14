#include "Song.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qcoreapplication.h>      // for QCoreApplication
#include <qfile.h>                 // for QFile
#include <qiodevice.h>             // for QIODevice
#include <qiodevicebase.h>         // for QIODeviceBase::ReadOnly, QIODevice...
#include <qjsondocument.h>         // for QJsonDocument
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValueRef
#include <qmessagebox.h>           // for QMessageBox
#include <QJsonDocument>

#include <algorithm>  // for copy, max
#include <iterator>   // for move_iterator, make_move_iterator

#include "NoteChord.h"  // for NoteChord, beats_column, denominat...
#include "Utilities.h"  // for has_instrument, cannot_open_error
class QObject;          // lines 19-19
class CellChange;

Song::Song(QObject *parent)
    : QAbstractItemModel(parent),
      root(TreeNode(instrument_pointers, default_instrument)) {
  extract_instruments(instrument_pointers, orchestra_text);
  verify_instruments(instrument_pointers, false);
}

auto Song::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto Song::data(const QModelIndex &index, int role) const -> QVariant {
  // assume the index is valid because qt is requesting data for it
  const auto &node = const_node_from_index(index);
  if (node.get_level() == root_level) {
    error_root();
    return {};
  }
  return node.note_chord_pointer->data(index.column(), role);
}

auto Song::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  const auto &node = const_node_from_index(index);
  if (node.get_level() == root_level) {
    error_root();
    return {};
  }
  return node.note_chord_pointer->flags(index.column());
}

auto Song::headerData(int section, Qt::Orientation orientation, int role) const
    -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section == symbol_column) {
      return {};
    }
    if (section == numerator_column) {
      return "Numerator";
    };
    if (section == denominator_column) {
      return "Denominator";
    };
    if (section == octave_column) {
      return "Octave";
    };
    if (section == beats_column) {
      return "Beats";
    };
    if (section == volume_percent_column) {
      return "Volume Percent";
    };
    if (section == tempo_percent_column) {
      return "Tempo Percent";
    };
    if (section == words_column) {
      return "Words";
    };
    if (section == instrument_column) {
      return "Instrument";
    };
    error_column(section);
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto Song::node_from_index(const QModelIndex &index) -> TreeNode & {
  if (!index.isValid()) {
    // an invalid index points to the root
    return root;
  }
  return *(static_cast<TreeNode *>(index.internalPointer()));
}

auto Song::const_node_from_index(const QModelIndex &index) const
    -> const TreeNode & {
  if (!index.isValid()) {
    // an invalid index points to the root
    return root;
  }
  return *(static_cast<TreeNode *>(index.internalPointer()));
}

// get a child index
auto Song::index(int row, int column, const QModelIndex &parent_index) const
    -> QModelIndex {
  // createIndex needs a pointer to the item, not the parent
  // will error if row doesn't exist
  return createIndex(row, column,
                    const_node_from_index(parent_index).child_pointers[row].get());
}

// get the parent index
auto Song::parent(const QModelIndex &index) const -> QModelIndex {
  const auto &node = const_node_from_index(index);
  if (node.get_level() == root_level) {
    error_root();
    return {};
  }
  auto &parent_node = *(node.parent_pointer);
  if (parent_node.get_level() == 0) {
    // root has an invalid index
    return {};
  }
  // always point to the nested first column of the parent
  return createIndex(parent_node.is_at_row(), 0, &parent_node);
}

auto Song::rowCount(const QModelIndex &parent_index) const -> int {
  const auto &parent_node = const_node_from_index(parent_index);
  // column will be invalid for the root
  // we are only nesting into the first column of notes
  if (parent_node.get_level() == 0 || parent_index.column() == 0) {
    return static_cast<int>(parent_node.get_child_count());
  }
  return 0;
}

// node will check for errors, so no need to check for errors here
void Song::setData_directly(const QModelIndex &index,
                            const QVariant &new_value) {
  auto &node = node_from_index(index);
  if (node.get_level() == root_level) {
    error_root();
    return;
  }
  node.note_chord_pointer->setData(index.column(), new_value);
  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

auto Song::setData(const QModelIndex &index, const QVariant &new_value,
                   int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }
  undo_stack.push(new CellChange(*this, index, new_value));
  return true;
}

auto Song::removeRows_internal(size_t position, size_t rows,
                               const QModelIndex &parent_index) -> void {
  auto &parent_node = node_from_index(parent_index);
  if (!(parent_node.verify_child_at(position) && parent_node.verify_child_at(position + rows - 1))) {
    return;
  };
  parent_node.child_pointers.erase(
      parent_node.child_pointers.begin() + static_cast<int>(position),
      parent_node.child_pointers.begin() + static_cast<int>(position + rows));
};

// node will check for errors, so no need to check here
auto Song::removeRows(int position, int rows, const QModelIndex &parent_index)
    -> bool {
  beginRemoveRows(parent_index, position, position + rows - 1);
  removeRows_internal(position, rows, parent_index);
  endRemoveRows();
  return true;
};

// use additional deleted_rows to save deleted rows
// node will check for errors, so no need to check here
auto Song::remove_save(size_t position, size_t rows,
                       const QModelIndex &parent_index,
                       std::vector<std::unique_ptr<TreeNode>> &deleted_rows)
    -> void {
  auto &node = node_from_index(parent_index);
  auto &child_pointers = node.child_pointers;
  if (!(node.verify_child_at(position) && node.verify_child_at(position + rows - 1))) {
    return;
  };
  beginRemoveRows(parent_index, static_cast<int>(position),
                  static_cast<int>(position + rows - 1));
  deleted_rows.insert(
      deleted_rows.begin(),
      std::make_move_iterator(child_pointers.begin() +
                              static_cast<int>(position)),
      std::make_move_iterator(child_pointers.begin() +
                              static_cast<int>(position + rows)));
  auto &parent_node = node_from_index(parent_index);
  removeRows_internal(position, rows, parent_index);
  endRemoveRows();
}

auto Song::insertRows(int position, int rows, const QModelIndex &parent_index)
    -> bool {
  
  auto &node = node_from_index(parent_index);
  auto &child_pointers = node.child_pointers;
  // will error if invalid
  if (!(node.verify_insertable_at(position))) {
    return false;
  };
  beginInsertRows(parent_index, position, position + rows - 1);
  for (int index = position; index < position + rows; index = index + 1) {
    // will error if childless
    child_pointers.insert(child_pointers.begin() + index,
                          std::make_unique<TreeNode>(
                              instrument_pointers, default_instrument, &node));
  }
  endInsertRows();
  return true;
};

auto Song::insert_children(size_t position,
                           std::vector<std::unique_ptr<TreeNode>> &insertion,
                           const QModelIndex &parent_index) -> void {
  
  auto &parent_node = node_from_index(parent_index);

  if (!(parent_node.verify_insertable_at(position))) {
    return;
  }

  auto &child_pointers = parent_node.child_pointers;
  // will error if invalid
  auto child_level = parent_node.get_level() + 1;
  // make sure we are inserting the right level items
  for (const auto &child_pointer : insertion) {
    auto new_child_level = child_pointer->get_level();
    if (child_level != new_child_level) {
      // TODO: test
      qCritical("Level mismatch between level %d and new level %d!",
                child_level, new_child_level);
      return;
    }
  }
  
  beginInsertRows(parent_index, static_cast<int>(position),
                  static_cast<int>(position + insertion.size() - 1));
  child_pointers.insert(
      parent_node.child_pointers.begin() + static_cast<int>(position),
      std::make_move_iterator(insertion.begin()),
      std::make_move_iterator(insertion.end()));
  insertion.clear();
  endInsertRows();
};

auto Song::to_json() -> QJsonDocument {
  QJsonObject json_object;
  json_object["frequency"] = frequency;
  json_object["tempo"] = tempo;
  json_object["volume_percent"] = volume_percent;
  json_object["default_instrument"] = default_instrument;
  json_object["orchestra_text"] = orchestra_text;
  root.save_children(json_object);
  return QJsonDocument(json_object);
}

auto Song::load_from(const QByteArray &song_text) -> bool {
  const QJsonDocument document = QJsonDocument::fromJson(song_text);
  if (document.isNull()) {
    QMessageBox::warning(nullptr, "JSON parsing error", "Cannot parse JSON!");
    return false;
  }
  if (!(document.isObject())) {
    QMessageBox::warning(nullptr, "JSON parsing error", "Expected JSON object!");
    return false;
  }
  auto json_object = document.object();
  if (!verify_json(json_object)) {
    return false;
  }
  frequency = json_object["frequency"].toInt();
  volume_percent = json_object["volume_percent"].toInt();
  tempo = json_object["tempo"].toDouble();
  default_instrument = json_object["default_instrument"].toString();
  orchestra_text = json_object["orchestra_text"].toString();

  instrument_pointers.clear();
  extract_instruments(instrument_pointers, orchestra_text);

  beginResetModel();
  root.child_pointers.clear();
  root.load_children(json_object);
  endResetModel();
  return true;
}

void Song::redisplay() {
  beginResetModel();
  endResetModel();
}

auto Song::verify_instruments(
  std::vector<std::unique_ptr<const QString>> &new_instrument_pointers,
  bool interactive
)
    -> bool {
  if (!has_instrument(new_instrument_pointers, default_instrument)) {
    error_instrument(default_instrument, interactive);
    return false;
  }
  for (auto &chord_pointer : root.child_pointers) {
    for (auto &note_pointer : chord_pointer->child_pointers) {
      auto instrument = note_pointer->note_chord_pointer->get_instrument();
      if (!has_instrument(new_instrument_pointers, instrument)) {
        error_instrument(default_instrument, interactive);
        return false;
      }
    }
  }
  return true;
}
