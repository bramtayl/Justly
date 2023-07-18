#include "Song.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qiodevicebase.h>         // for QIODeviceBase::OpenMode, QIODevice...
#include <qjsonarray.h>     // for QJsonArray, QJsonArray::iterator
#include <qjsondocument.h>  // for QJsonDocument
#include <qjsonobject.h>    // for QJsonObject
#include <qjsonvalue.h>     // for QJsonValueRef, QJsonValue
#include <qlist.h>          // for QList
#include <qtextstream.h>           // for QTextStream, operator<<, endl

#include <algorithm>  // for all_of

#include "Chord.h"           // for Chord
#include "TreeNode.h"        // for TreeNode
#include "src/Instrument.h"  // for Instrument
#include "utilities.h"       // for require_json_field, parse_error

Song::Song(const QString &starting_instrument_input)
    : starting_instrument(starting_instrument_input) {
  if (!has_instrument(starting_instrument_input)) {
    qCritical("Cannot find starting instrument \"%s\"!",
              qUtf8Printable(starting_instrument_input));
  }
}

auto Song::to_json() const -> QJsonDocument {
  QJsonObject json_object;
  json_object["starting_key"] = starting_key;
  json_object["starting_tempo"] = starting_tempo;
  json_object["starting_volume"] = starting_volume;
  json_object["starting_instrument"] = starting_instrument;
  root.save_to(json_object);
  return QJsonDocument(json_object);
}

auto Song::load_text(const QByteArray &song_text) -> bool {
  const QJsonDocument document = QJsonDocument::fromJson(song_text);
  if (!verify_json_document(document)) {
    return false;
  }
  if (!(document.isObject())) {
    parse_error("Expected JSON object!");
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

  root.load_from(json_object);
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
                verify_json_instrument(json_song, field_name))) {
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
                               return Chord::verify_json(*this, chord_value);
                             });
        } else {
          warn_unrecognized_field("song", field_name);
          return false;
        }
        return true;
      });
  return true;
};

auto Song::get_instrument_id(const QString &name) const -> int {
  for (const auto &instrument : instruments) {
    if (instrument.name == name) {
      return instrument.id;
    }
  }
  qCritical("Cannot find instrument \"%s\"!",
            qUtf8Printable(name));
  return -1;
}

auto Song::has_instrument(const QString &maybe_instrument) const -> bool {
  return std::any_of(instruments.cbegin(), instruments.cend(),
                     [&maybe_instrument](const auto &instrument) {
                       return instrument.name == maybe_instrument;
                     });
}

auto Song::verify_json_instrument(const QJsonObject &json_object,
                                  const QString &field_name) const -> bool {
  const auto json_value = json_object[field_name];
  if (!(verify_json_string(json_value, field_name))) {
    return false;
  }
  const auto maybe_instrument = json_value.toString();
  if (!has_instrument(maybe_instrument)) {
    parse_error(
        QString("Cannot find %1 instrument \"%2\"").arg(field_name).arg(maybe_instrument));
    return false;
  }
  return true;
}

auto Song::get_orchestra_code() const -> QByteArray {
  QByteArray orchestra_code = "";
  QTextStream orchestra_io(&orchestra_code, QIODeviceBase::WriteOnly);
  orchestra_io << R"(
nchnls = 2
0dbfs = 1

gisound_font sfload ")"
               << soundfont_file << R"("
; because 0dbfs = 1, not 32767, I guess
gibase_amplitude = 1/32767
; velocity is how hard you hit the key (not how loud it is)
gimax_velocity = 127
; short release
girelease_duration = 0.05

; arguments p1 = instrument, p2 = start_time, p3 = duration, p4 = instrument_number, p5 = frequency, p6 = amplitude (max 1)
instr play_soundfont
  ; assume velociy is proportional to amplitude
  ; arguments velocity, midi number, amplitude, frequency, preset number, ignore midi flag
  aleft_sound, aright_sound sfplay3 gimax_velocity * p7, p6, gibase_amplitude * p7, p5, p4, 1
  ; arguments start_level, sustain_duration, mid_level, release_duration, end_level
  acutoff_envelope linsegr 1, p3, 1, girelease_duration, 0
  ; cutoff instruments at end of the duration
  aleft_sound_cut = aleft_sound * acutoff_envelope
  aright_sound_cut = aright_sound * acutoff_envelope
  outs aleft_sound_cut, aright_sound_cut
endin

instr clear_events
    turnoff3 nstrnum("play_soundfont")
endin
)";

  for (int index = 0; index < instruments.size(); index = index + 1) {
    const auto &instrument = instruments[index];
    orchestra_io << "gi" << instrument.code << " sfpreset "
                 << instrument.preset_number << ", " << instrument.bank_number
                 << ", gisound_font, " << instrument.id << Qt::endl;
  }
  orchestra_io.flush();
  return orchestra_code;
}