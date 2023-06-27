#include "Song.h"

#include <algorithm>               // for all_of
#include <QtCore/qglobal.h>       // for qCritical
#include <QtCore/qtcoreexports.h> // for qUtf8Printable
#include <csound/csound.hpp>      // for Csound
#include <qbytearray.h>           // for QByteArray
#include <qjsonarray.h>           // for QJsonArray
#include <qjsondocument.h>        // for QJsonDocument
#include <qjsonobject.h>          // for QJsonObject
#include <qjsonvalue.h>           // for QJsonValueRef, QJsonValue
#include <qlist.h>                // for QList, QList<>::iterator
#include <qmessagebox.h>          // for QMessageBox

#include "TreeNode.h"  // for TreeNode
#include "Utilities.h" // for require_json_field, extract_instru...

class QUndoStack;

Song::Song(Csound &csound_session_input, QUndoStack &undo_stack_input,
           const QString &starting_instrument_input,
           const QString &orchestra_code_input)
    : csound_session(csound_session_input), undo_stack(undo_stack_input),
      starting_instrument(starting_instrument_input),
      orchestra_code(orchestra_code_input) {

  extract_instruments(instrument_pointers, orchestra_code_input);
  if (!has_instrument(instrument_pointers, starting_instrument_input)) {
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
  json_object["orchestra_code"] = orchestra_code;
  auto chord_count = chords_model_pointer->root.get_child_count();
  if (chord_count > 0) {
    json_object["chords"] = chords_model_pointer->save();
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

  if (json_object.contains("chords")) {
    chords_model_pointer->load(json_object["chords"].toArray());
  }
  return true;
}

auto Song::verify_orchestra_code_compiles(const QString &new_orchestra_text)
    -> bool {
  // test the orchestra
  auto orchestra_error_code =
      csound_session.CompileOrc(qUtf8Printable(new_orchestra_text));
  if (orchestra_error_code != 0) {
    QMessageBox::warning(nullptr, "Orchestra warning",
                         QString("Cannot compile orchestra, error code %1! Not "
                                 "changing orchestra text")
                             .arg(orchestra_error_code));
    return false;
  }
  csound_session.CompileOrc(qUtf8Printable(orchestra_code));
  return true;
  // make sure to compile the existing code
}

void Song::set_orchestra_code(const QString &new_orchestra_text) {
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
  if (!verify_orchestra_code_compiles(new_orchestra_text)) {
    return false;
  }
  std::vector<std::unique_ptr<const QString>> new_instrument_pointers;
  extract_instruments(new_instrument_pointers, new_orchestra_text);
  auto keys = json_song.keys();
  return std::all_of(
      keys.cbegin(), keys.cend(),
      [&json_song, &new_instrument_pointers](const auto &field_name) {
        if (field_name == "starting_instrument") {
          if (!(require_json_field(json_song, field_name) &&
                verify_json_instrument(new_instrument_pointers, json_song,
                                       field_name))) {
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
          if (!(ChordsModel::verify_json(chords_value.toArray(),
                                         new_instrument_pointers))) {
            return false;
          };
        } else if (field_name == "orchestra_code") {
        } else {
          warn_unrecognized_field("song", field_name);
          return false;
        }
        return true;
      });
  return true;
};
