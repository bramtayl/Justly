#include "Song.h"

#include <QtCore/qglobal.h>      // for qCritical
#include <qbytearray.h>          // for QByteArray
#include <qfile.h>               // for QFile
#include <qiodevice.h>           // for QIODevice
#include <qiodevicebase.h>       // for QIODeviceBase::ReadOnly, QIODeviceBa...
#include <qjsondocument.h>       // for QJsonDocument
#include <qjsonobject.h>         // for QJsonObject
#include <qjsonvalue.h>          // for QJsonValueRef
#include <qregularexpression.h>  // for QRegularExpressionMatchIteratorRange...

#include <algorithm>  // for copy, max
#include <iterator>   // for move_iterator, make_move_iterator
#include <utility>    // for move

#include "JsonHelpers.h"  // for get_positive_int, get_string, get_no...
#include "NoteChord.h"    // for NoteChord, beats_column, denominator...
class QObject;            // lines 19-19

auto get_instruments(const QString &orchestra_text)
    -> std::vector<std::unique_ptr<const QString>> {
  std::vector<std::unique_ptr<const QString>> instruments;
  QRegularExpression const instrument_pattern(R"(\binstr\s+\b(\w+)\b)");
  QRegularExpressionMatchIterator const instrument_matches =
      instrument_pattern.globalMatch(orchestra_text);
  for (const QRegularExpressionMatch &match : instrument_matches) {
    instruments.push_back(
        std::move(std::make_unique<QString>(match.captured(1))));
  }
  return instruments;
}

Song::Song(QObject *parent)
    : QAbstractItemModel(parent),
      instruments(get_instruments(DEFAULT_ORCHESTRA_TEXT)),
      root(TreeNode(instruments, default_instrument)) {}

auto Song::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto Song::data(const QModelIndex &index, int role) const -> QVariant {
  // assume the index is valid because qt is requesting data for it
  const auto &node = const_node_from_index(index);
  node.assert_not_root();
  return node.note_chord_pointer->data(index.column(), role);
}

auto Song::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  const auto &node = const_node_from_index(index);
  node.assert_not_root();
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
    if (section == volume_ratio_column) {
      return "Volume Ratio";
    };
    if (section == tempo_ratio_column) {
      return "Tempo Ratio";
    };
    if (section == words_column) {
      return "Words";
    };
    if (section == instrument_column) {
      return "Instrument";
    };
    NoteChord::error_column(section);
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
                     &(const_node_from_index(parent_index).get_child(row)));
}

// get the parent index
auto Song::parent(const QModelIndex &index) const -> QModelIndex {
  const auto &node = const_node_from_index(index);
  node.assert_not_root();
  auto &parent_node = node.get_parent();
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
auto Song::setData_directly(const QModelIndex &index, const QVariant &new_value,
                            int role) -> bool {
  auto &node = node_from_index(index);
  node.assert_not_root();
  auto was_set =
      node.note_chord_pointer->setData(index.column(), new_value, role);
  if (was_set) {
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
  }
  return was_set;
}

auto Song::setData(const QModelIndex &index, const QVariant &new_value,
                   int role) -> bool {
  emit set_data_signal(index, new_value, role);
  return true;
}

auto Song::removeRows_internal(size_t position, size_t rows,
                               const QModelIndex &parent_index) -> void {
  auto &parent_node = node_from_index(parent_index);
  parent_node.assert_child_at(position);
  parent_node.assert_child_at(position + rows - 1);
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
  beginRemoveRows(parent_index, static_cast<int>(position),
                  static_cast<int>(position + rows - 1));
  auto &node = node_from_index(parent_index);
  auto &child_pointers = node.child_pointers;
  node.assert_child_at(position);
  node.assert_child_at(position + rows - 1);
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
  beginInsertRows(parent_index, position, position + rows - 1);
  auto &node = node_from_index(parent_index);
  auto &child_pointers = node.child_pointers;
  // will error if invalid
  node.assert_insertable_at(position);
  for (int row = 0; row < rows; row = row + 1) {
    // will error if childless
    child_pointers.insert(
        child_pointers.begin() + position + row,
        std::make_unique<TreeNode>(instruments, default_instrument, &node));
  }
  endInsertRows();
  return true;
};

auto Song::insert_children(size_t position,
                           std::vector<std::unique_ptr<TreeNode>> &insertion,
                           const QModelIndex &parent_index) -> void {
  beginInsertRows(parent_index, static_cast<int>(position),
                  static_cast<int>(position + insertion.size() - 1));
  auto &parent_node = node_from_index(parent_index);
  auto &child_pointers = parent_node.child_pointers;
  // will error if invalid
  auto child_level = parent_node.get_level() + 1;
  // make sure we are inserting the right level items
  for (const auto &child_pointer : child_pointers) {
    auto new_child_level = child_pointer->get_level();
    if (child_level != new_child_level) {
      // TODO: test
      qCritical("Level mismatch between level %d and new level %d!",
                child_level, new_child_level);
      return;
    }
  }
  parent_node.assert_insertable_at(position);
  child_pointers.insert(
      parent_node.child_pointers.begin() + static_cast<int>(position),
      std::make_move_iterator(insertion.begin()),
      std::make_move_iterator(insertion.end()));
  insertion.clear();
  endInsertRows();
};

void Song::save_to(const QString &file) const {
  QFile output(file);
  if (output.open(QIODevice::WriteOnly)) {
    QJsonObject json_object;
    json_object["frequency"] = frequency;
    json_object["tempo"] = tempo;
    json_object["volume_percent"] = volume_percent;
    json_object["default_instrument"] = default_instrument;
    json_object["orchestra_text"] = orchestra_text;
    root.save_children(json_object);
    output.write(QJsonDocument(json_object).toJson());
    output.close();
  }
}

void Song::load_from(const QString &file) {
  QFile input(file);
  if (input.open(QIODevice::ReadOnly)) {
    auto document = QJsonDocument::fromJson(input.readAll());
    if (document.isNull()) {
      qCritical("Parse error!");
      return;
    }
    if (!(document.isObject())) {
      TreeNode::error_not_object();
      return;
    }
    auto json_object = document.object();

    frequency = get_positive_int(json_object, "frequency", DEFAULT_FREQUENCY);
    volume_percent = get_non_negative_int(json_object, "volume_percent",
                                          DEFAULT_VOLUME_PERCENT);
    tempo = get_positive_int(json_object, "tempo", DEFAULT_TEMPO);
    default_instrument =
        get_string(json_object, "default_instrument", default_instrument);
    orchestra_text = get_string(json_object, "orchestra_text", "");

    beginResetModel();
    root.child_pointers.clear();
    root.load_children(json_object);
    endResetModel();
    input.close();
  } else {
    QByteArray raw_string = file.toLocal8Bit();
    qCritical("Cannot open file %s", raw_string.data());
  }
}

void Song::set_default_instrument(QString default_instrument_input) {
  beginResetModel();
  default_instrument = default_instrument_input;
  endResetModel();
} 