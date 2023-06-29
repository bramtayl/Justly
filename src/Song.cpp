#include "Song.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qjsonarray.h>            // for QJsonArray
#include <qjsondocument.h>         // for QJsonDocument
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValueRef
#include <qlist.h>                 // for QList

#include <algorithm>  // for all_of
#include <memory>                  // for unique_ptr, make_unique
#include <utility>                 // for move

#include "Chord.h"
#include "NoteChord.h"             // for NoteChord
#include "TreeNode.h"   // for TreeNode
#include "utilities.h"  // for require_json_field, json_parse_error

Song::Song(const QString &starting_instrument_input)
    : starting_instrument(starting_instrument_input), root(instruments) {
  if (!has_instrument(instruments, starting_instrument_input)) {
    qCritical("Cannot find starting instrument %s",
              qUtf8Printable(starting_instrument_input));
    return;
  }
}

auto Song::to_json() const -> QJsonDocument {
  QJsonObject json_object;
  json_object["starting_key"] = starting_key;
  json_object["starting_tempo"] = starting_tempo;
  json_object["starting_volume"] = starting_volume;
  json_object["starting_instrument"] = starting_instrument;
  auto chord_count = root.get_child_count();
  if (chord_count > 0) {
    QJsonArray json_chords;
    for (const auto &chord_node_pointer : root.child_pointers) {
      QJsonObject json_chord;
      chord_node_pointer->note_chord_pointer->save(json_chord);

      if (!(chord_node_pointer->child_pointers.empty())) {
        QJsonArray note_array;
        for (const auto &note_node_pointer : chord_node_pointer->child_pointers) {
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

  if (json_object.contains("chords")) {
    root.child_pointers.clear();
    auto json_chords = json_object["chords"].toArray();
    for (const auto &chord_value : json_chords) {
      const auto &json_chord = chord_value.toObject();
      auto chord_node_pointer = std::make_unique<TreeNode>(instruments, &root);
      chord_node_pointer->note_chord_pointer->load(json_chord);

      if (json_chord.contains("notes")) {
        for (const auto &note_node : json_chord["notes"].toArray()) {
          const auto &json_note = note_node.toObject();
          auto note_node_pointer =
              std::make_unique<TreeNode>(instruments, chord_node_pointer.get());
          note_node_pointer->note_chord_pointer->load(json_note);
          chord_node_pointer->child_pointers.push_back(
              std::move(note_node_pointer));
        }
      }
      root.child_pointers.push_back(std::move(chord_node_pointer));
    }
  }
  return true;
}

auto Song::verify_json(const QJsonObject &json_song) -> bool {
  if (!(require_json_field(json_song, "starting_instrument") &&
        require_json_field(json_song, "starting_key") &&
        require_json_field(json_song, "starting_volume") &&
        require_json_field(json_song, "starting_tempo"))) {
    return false;
  }
  auto keys = json_song.keys();
  return std::all_of(
      keys.cbegin(), keys.cend(), [&json_song, this](const auto &field_name) {
        if (field_name == "starting_instrument") {
          if (!(require_json_field(json_song, field_name) &&
                verify_json_instrument(instruments, json_song, field_name))) {
            return false;
          }
        } else if (field_name == "starting_key") {
          if (!(require_json_field(json_song, field_name) &&
                verify_bounded_double(json_song, field_name,
                                      MINIMUM_STARTING_KEY,
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
                verify_bounded_double(json_song, field_name,
                                      MINIMUM_STARTING_TEMPO,
                                      MAXIMUM_STARTING_TEMPO))) {
            return false;
          }
        } else if (field_name == "chords") {
          auto chords_value = json_song[field_name];
          if (!(verify_json_array(chords_value, "chords"))) {
            return false;
          }
          auto json_chords = chords_value.toArray();
          return std::all_of(json_chords.cbegin(), json_chords.cend(),
                     [this](const auto &chord_value) {
                       if (!(verify_json_object(chord_value, "chord"))) {
                         return false;
                       }
                       const auto json_chord = chord_value.toObject();
                       return Chord::verify_json(json_chord, instruments);
                     });
        } else {
          warn_unrecognized_field("song", field_name);
          return false;
        }
        return true;
      });
  return true;
};

auto Song::get_instrument_code(const QString &name) -> QString {
  for (const auto &instrument : instruments) {
    if (instrument.name == name) {
      return instrument.code;
    }
  }
  qCritical("Cannot find instrument with display name %s",
            qUtf8Printable(name));
  return {};
}
