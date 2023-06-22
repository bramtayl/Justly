#include "Song.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qcontainerfwd.h>         // for QStringList
#include <qjsonarray.h>            // for QJsonArray, QJsonArray::iterator
#include <qjsondocument.h>         // for QJsonDocument
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValueRef, QJsonValue
#include <qlist.h>                 // for QList, QList<>::iterator
#include <qmessagebox.h>           // for QMessageBox

#include <algorithm>  // for copy, max
#include <iterator>   // for move_iterator, make_move_iterator
#include <utility>    // for move

#include "Chord.h"      // for Chord
#include "commands.h"
#include "NoteChord.h"  // for NoteChord, symbol_column, beats_co...
#include "Utilities.h"  // for require_json_field, extract_instru...

class QObject;  // lines 19-19

Song::Song(const QString &starting_instrument_input, const QString &orchestra_code_input,
           QObject *parent_input)
    : QAbstractItemModel(parent_input),
      starting_instrument(starting_instrument_input),
      orchestra_code(orchestra_code_input),
      root(TreeNode(instrument_pointers)) {
  csound_session.SetOption("--output=devaudio");
  csound_session.SetOption("--messagelevel=16");

  extract_instruments(instrument_pointers, orchestra_code_input);
  if (!has_instrument(instrument_pointers, starting_instrument_input)) {
    qCritical("Cannot find starting instrument %s",
              qUtf8Printable(starting_instrument_input));
    return;
  }

  auto orchestra_error_code =
      csound_session.CompileOrc(qUtf8Printable(orchestra_code_input));
  if (orchestra_error_code != 0) {
    qCritical("Cannot compile orchestra, error code %d", orchestra_error_code);
    return;
  }

  csound_session.Start();
}

Song::~Song() {
  performance_thread.Stop();
  performance_thread.Join();
}

auto Song::columnCount(const QModelIndex & /*parent*/) const -> int {
  return NOTE_CHORD_COLUMNS;
}

auto Song::data(const QModelIndex &index, int role) const -> QVariant {
  // assume the index is valid because qt is requesting data for it
  const auto &node = const_node_from_index(index);
  if (!(node.verify_not_root())) {
    return {};
  }
  return node.note_chord_pointer->data(index.column(), role);
}

auto Song::flags(const QModelIndex &index) const -> Qt::ItemFlags {
  const auto &node = const_node_from_index(index);
  if (!(node.verify_not_root())) {
    return {};
  }
  auto column = index.column();
  if (column == symbol_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
  if (column == numerator_column || column == denominator_column ||
      column == octave_column || column == beats_column ||
      column == volume_percent_column || column == tempo_percent_column ||
      column == words_column || column == instrument_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
  error_column(column);
  return Qt::NoItemFlags;
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
  const auto &parent_node = const_node_from_index(parent_index);
  if (!(parent_node.verify_child_at(row))) {
    return {};
  }
  return createIndex(row, column, parent_node.child_pointers[row].get());
}

// get the parent index
auto Song::parent(const QModelIndex &index) const -> QModelIndex {
  const auto &node = const_node_from_index(index);
  if (!(node.verify_not_root())) {
    return {};
  }
  auto *parent_node_pointer = node.parent_pointer;
  if (parent_node_pointer->is_root()) {
    // root has an invalid index
    return {};
  }
  // always point to the nested first column of the parent
  return createIndex(parent_node_pointer->is_at_row(), 0, parent_node_pointer);
}

auto Song::rowCount(const QModelIndex &parent_index) const -> int {
  const auto &parent_node = const_node_from_index(parent_index);
  // column will be invalid for the root
  // we are only nesting into the first column of notes
  if (parent_node.is_root() || parent_index.column() == symbol_column) {
    return static_cast<int>(parent_node.get_child_count());
  }
  return 0;
}

// node will check for errors, so no need to check for errors here
void Song::setData_directly(const QModelIndex &index,
                            const QVariant &new_value) {
  auto &node = node_from_index(index);
  if (!(node.verify_not_root())) {
    return;
  }
  if (node.note_chord_pointer->setData(index.column(), new_value)) {
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
  }
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
  if (!(parent_node.verify_child_at(position) &&
        parent_node.verify_child_at(position + rows - 1))) {
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
  if (!(node.verify_child_at(position) &&
        node.verify_child_at(position + rows - 1))) {
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
                              instrument_pointers, &node));
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

  // will error if invalid
  auto child_level = parent_node.get_level() + 1;
  // make sure we are inserting the right level items
  for (const auto &new_child_pointer : insertion) {
    auto new_child_level = new_child_pointer->get_level();
    if (child_level != new_child_level) {
      // TODO: test
      qCritical("Level mismatch between level %d and new level %d!",
                child_level, new_child_level);
      return;
    }
  }

  beginInsertRows(parent_index, static_cast<int>(position),
                  static_cast<int>(position + insertion.size() - 1));
  auto &child_pointers = parent_node.child_pointers;
  child_pointers.insert(child_pointers.begin() + static_cast<int>(position),
                        std::make_move_iterator(insertion.begin()),
                        std::make_move_iterator(insertion.end()));
  insertion.clear();
  endInsertRows();
};

auto Song::to_json() const -> QJsonDocument {
  QJsonObject json_object;
  json_object["starting_key"] = starting_key;
  json_object["starting_tempo"] = starting_tempo;
  json_object["starting_volume"] = starting_volume;
  json_object["starting_instrument"] = starting_instrument;
  json_object["orchestra_code"] = orchestra_code;
  auto chord_count = root.get_child_count();
  if (chord_count > 0) {
    QJsonArray json_chords;
    for (const auto &chord_node_pointer : root.child_pointers) {
      QJsonObject json_chord;
      chord_node_pointer->note_chord_pointer->save(json_chord);

      if (!(chord_node_pointer->child_pointers.empty())) {
        QJsonArray note_array;
        for (const auto &note_node_pointer :
             chord_node_pointer->child_pointers) {
          QJsonObject json_note;
          note_node_pointer->note_chord_pointer->save(json_note);
          note_array.push_back(std::move(json_note));
        }
        json_chord["notes"] = std::move(note_array);
      }
      json_chords.push_back(std::move(json_chord));
    }
    json_object["chords"] = std::move(json_chords);
  }
  return QJsonDocument(json_object);
}

auto Song::load_from(const QByteArray &song_text) -> bool {
  const QJsonDocument document = QJsonDocument::fromJson(song_text);
  if (document.isNull()) {
    json_parse_error("Cannot parse JSON!");
    return false;
  }
  if (!(document.isObject())) {
    json_parse_error("Expected JSON object!");
    return false;
  }
  auto json_object = document.object();
  if (!verify_json(json_object)) {
    return false;
  }
  starting_key = json_object["starting_key"].toDouble();
  starting_volume = json_object["starting_volume"].toDouble();
  starting_tempo = json_object["starting_tempo"].toDouble();
  starting_instrument = json_object["starting_instrument"].toString();
  orchestra_code = json_object["orchestra_code"].toString();

  instrument_pointers.clear();
  extract_instruments(instrument_pointers, orchestra_code);

  beginResetModel();
  root.child_pointers.clear();

  if (json_object.contains("chords")) {
    for (const auto &chord_node : json_object["chords"].toArray()) {
      const auto &json_chord = chord_node.toObject();
      auto chord_node_pointer = std::make_unique<TreeNode>(
          instrument_pointers, &root);
      chord_node_pointer->note_chord_pointer->load(json_chord);

      if (json_chord.contains("notes")) {
        for (const auto &note_node : json_chord["notes"].toArray()) {
          const auto &json_note = note_node.toObject();
          auto note_node_pointer = std::make_unique<TreeNode>(
              instrument_pointers,
              chord_node_pointer.get());
          note_node_pointer->note_chord_pointer->load(json_note);
          chord_node_pointer->child_pointers.push_back(
              std::move(note_node_pointer));
        }
      }
      root.child_pointers.push_back(std::move(chord_node_pointer));
    }
  }
  endResetModel();
  return true;
}

void Song::redisplay() {
  beginResetModel();
  endResetModel();
}

auto Song::verify_instruments(
    std::vector<std::unique_ptr<const QString>> &new_instrument_pointers)
    -> bool {
  for (auto &chord_node_pointer : root.child_pointers) {
    for (auto &note_node_pointer : chord_node_pointer->child_pointers) {
      auto instrument = note_node_pointer->note_chord_pointer->instrument;
      if (!has_instrument(new_instrument_pointers, instrument)) {
        error_instrument(instrument);
        return false;
      }
    }
  }
  return true;
}

void Song::stop_playing() {
  performance_thread.Pause();
  performance_thread.FlushMessageQueue();
  csound_session.RewindScore();
}

void Song::play(int position, size_t rows, const QModelIndex &parent_index) {
  stop_playing();

  current_key = starting_key;
  current_volume = (FULL_NOTE_VOLUME * starting_volume) / PERCENT;
  current_tempo = starting_tempo;
  current_time = 0.0;
  current_instrument = starting_instrument;

  auto end_position = position + rows;
  auto &parent = node_from_index(parent_index);
  if (!(parent.verify_child_at(position) &&
        parent.verify_child_at(end_position - 1))) {
    return;
  };
  auto &sibling_pointers = parent.child_pointers;
  auto parent_level = parent.get_level();
  if (parent.is_root()) {
    for (auto index = 0; index < position; index = index + 1) {
      auto &sibling = *sibling_pointers[index];
      update_with_chord(sibling);
    }
    for (auto index = position; index < end_position; index = index + 1) {
      auto &sibling = *sibling_pointers[index];
      update_with_chord(sibling);
      for (const auto &nibling_pointer : sibling.child_pointers) {
        schedule_note(*nibling_pointer);
      }
      current_time = current_time +
                     get_beat_duration() * sibling.note_chord_pointer->beats;
    }
  } else if (parent_level == chord_level) {
    auto &grandparent = *(parent.parent_pointer);
    auto &uncle_pointers = grandparent.child_pointers;
    auto parent_position = parent.is_at_row();
    for (auto index = 0; index <= parent_position; index = index + 1) {
      update_with_chord(*uncle_pointers[index]);
    }
    for (auto index = position; index < end_position; index = index + 1) {
      schedule_note(*sibling_pointers[index]);
    }
  } else {
    error_level(parent_level);
  }

  performance_thread.Play();
}

void Song::update_with_chord(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  current_key = current_key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_percent / 100.0;
  current_tempo = current_tempo * note_chord_pointer->tempo_percent / 100.0;
  auto chord_instrument = note_chord_pointer -> instrument;
  if (chord_instrument != "") {
    current_instrument = chord_instrument;
  }
}

auto Song::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Song::schedule_note(const TreeNode &node) {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto& instrument = note_chord_pointer->instrument;
  if (instrument == "") {
    instrument = current_instrument;   
  }
  performance_thread.InputMessage(qUtf8Printable(
    QString("i \"%1\" %2 %3 %4 %5")
        .arg(instrument)
        .arg(current_time)
        .arg(get_beat_duration() * note_chord_pointer->beats *
              note_chord_pointer->tempo_percent / 100.0)
        .arg(current_key * node.get_ratio()

                  )
        .arg(current_volume * note_chord_pointer->volume_percent / 100.0)));
}

auto Song::verify_orchestra_text_compiles(const QString &new_orchestra_text)
    -> bool {
  // test the orchestra
  stop_playing();
  auto orchestra_error_code =
      csound_session.CompileOrc(qUtf8Printable(new_orchestra_text));
  if (orchestra_error_code != 0) {
    QMessageBox::warning(nullptr, "Orchestra warning",
                         QString("Cannot compile orchestra, error code %1! Not "
                                 "changing orchestra text")
                             .arg(orchestra_error_code));
    return false;
  }
  // undo, then redo later
  // TODO: only do this once?
  csound_session.CompileOrc(qUtf8Printable(orchestra_code));
  return true;
}

void Song::set_orchestra_text(const QString &new_orchestra_text) {
  orchestra_code = new_orchestra_text;
  instrument_pointers.clear();
  extract_instruments(instrument_pointers, new_orchestra_text);
  csound_session.CompileOrc(qUtf8Printable(new_orchestra_text));
}

auto Song::verify_json(const QJsonObject &json_song) -> bool {
  if (!(require_json_field(json_song, "orchestra_code") &&
        require_json_field(json_song, "starting_instrument") &&
        require_json_field(json_song, "starting_key") &&
        require_json_field(json_song, "starting_volume") &&
        require_json_field(json_song, "starting_tempo"))) {
    return false;
  }

  const auto orchestra_value = json_song["orchestra_code"];
  if (!verify_json_string(orchestra_value, "orchestra_code")) {
    return false;
  }

  auto new_orchestra_text = orchestra_value.toString();
  if (!verify_orchestra_text_compiles(new_orchestra_text)) {
    return false;
  }

  std::vector<std::unique_ptr<const QString>> new_instrument_pointers;
  extract_instruments(new_instrument_pointers, new_orchestra_text);

  for (const auto &field_name : json_song.keys()) {
    if (field_name == "starting_instrument") {
      if (!(require_json_field(json_song, field_name) &&
            verify_json_instrument(new_instrument_pointers, json_song,
                                   field_name, false))) {
        return false;
      }
    } else if (field_name == "starting_key") {
      if (!(require_json_field(json_song, field_name) &&
            verify_bounded_double(json_song, field_name, MINIMUM_STARTING_KEY,
                                  MAXIMUM_STARTING_KEY))) {
        return false;
      }
    } else if (field_name == "starting_volume") {
      if (!(require_json_field(json_song, field_name) &&
            verify_bounded_double(json_song, field_name,
                                  MINIMUM_STARTING_VOLUME,
                                  MAXIMUM_STARTING_VOLUME))) {
        return false;
      }
    } else if (field_name == "starting_tempo") {
      if (!(require_json_field(json_song, field_name) &&
            verify_bounded_double(json_song, field_name, MINIMUM_STARTING_TEMPO,
                                  MAXIMUM_STARTING_TEMPO))) {
        return false;
      }
    } else if (field_name == "chords") {
      auto chords_value = json_song[field_name];
      if (!(verify_json_array(chords_value, "chords"))) {
        return false;
      }
      for (const auto &chord_value : chords_value.toArray()) {
        if (!(verify_json_object(chord_value, "chord"))) {
          return false;
        }
        const auto json_chord = chord_value.toObject();
        if (!(Chord::verify_json(json_chord, new_instrument_pointers))) {
          return false;
        }
      }
    } else if (!(field_name == "orchestra_code")) {
      warn_unrecognized_field("song", field_name);
      return false;
    }
  }
  return true;
};
