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

Song::Song(const QString &starting_instrument_input, const QString &orchestra_code_input) :
      starting_instrument(starting_instrument_input),
      orchestra_code(orchestra_code_input) {
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
  auto &parent = chords_model_pointer->node_from_index(parent_index);
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
      if (!(ChordsModel::verify_json(chords_value.toArray(), new_instrument_pointers))) {
        return false;
      };
    } else if (!(field_name == "orchestra_code")) {
      warn_unrecognized_field("song", field_name);
      return false;
    }
  }
  return true;
};
