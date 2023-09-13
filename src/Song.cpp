#include "Song.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qjsondocument.h>         // for QJsonDocument
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValueRef, QJsonValue

#include <algorithm>         // for all_of
#include <map>               // for operator!=, operator==
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <nlohmann/detail/json_ref.hpp>  // for json_ref

#include "Chord.h"
#include "Instrument.h"  // for Instrument
#include "JsonErrorHandler.h"
#include "TreeNode.h"   // for TreeNode
#include "utilities.h"  // for require_json_field, parse_error

auto Song::get_validator() -> nlohmann::json_schema::json_validator & {
  static nlohmann::json_schema::json_validator validator(
    nlohmann::json({
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"title", "Song"},
      {"description", "A Justly song in JSON format"},
      {"type", "object"},
      {"required", {
        "starting_key",
        "starting_tempo",
        "starting_volume",
        "starting_instrument"
      }},
      {"properties", {
        {"starting_instrument", {
          {"type", "string"},
          {"description", "the starting instrument"},
          {"enum", Instrument::get_all_instrument_names()}
        }},
        {"starting_key", {
          {"type", "integer"},
          {"description", "the starting key, in Hz"},
          {"minimum", MINIMUM_STARTING_KEY},
          {"maximum", MAXIMUM_STARTING_KEY}
        }},
        {"starting_tempo", {
          {"type", "integer"},
          {"description", "the starting tempo, in bpm"},
          {"minimum", MINIMUM_STARTING_TEMPO},
          {"maximum", MAXIMUM_STARTING_TEMPO}
        }},
        {"starting_volume", {
          {"type", "integer"},
          {"description", "the starting volume, from 1 to 100"},
          {"minimum", MINIMUM_STARTING_VOLUME},
          {"maximum", MAXIMUM_STARTING_VOLUME}
        }},
        {"chords", {
          {"type", "array"},
          {"description", "a list of chords"},
          {"items", Chord::get_schema()}
        }}
      }}   
    })
  );
  return validator;
}

Song::Song(const QString &starting_instrument_input)
    : starting_instrument(starting_instrument_input) {
  if (!has_instrument(starting_instrument_input)) {
    qCritical("Cannot find starting instrument \"%s\"!",
              qUtf8Printable(starting_instrument_input));
  }
}

auto Song::to_json() const -> QJsonDocument {
  QJsonObject json_object;
  json_object["$schema"] =
      "https://raw.githubusercontent.com/bramtayl/Justly/"
      "master/src/song_schema.json";
  json_object["starting_key"] = starting_key;
  json_object["starting_tempo"] = starting_tempo;
  json_object["starting_volume"] = starting_volume;
  json_object["starting_instrument"] = starting_instrument;
  root.save_to(json_object);
  return QJsonDocument(json_object);
}

auto Song::load_text(const QByteArray &song_text) -> bool {
  nlohmann::json parsed_json;
  if (!(parse_json(parsed_json, song_text))) {
    return false;
  }

  JsonErrorHandler error_handler;
  get_validator().validate(parsed_json, error_handler);

  if (error_handler) {
    return false;
  }

  const QJsonDocument document = QJsonDocument::fromJson(song_text);

  auto json_object = document.object();
  starting_key = json_object["starting_key"].toDouble();
  starting_volume = json_object["starting_volume"].toDouble();
  starting_tempo = json_object["starting_tempo"].toDouble();
  starting_instrument = json_object["starting_instrument"].toString();

  root.load_from(json_object);
  return true;
}

auto Song::get_instrument_id(const QString &instrument_name) const -> int {
  for (const auto &instrument_pointer : instrument_pointers) {
    if (instrument_pointer -> instrument_name == instrument_name) {
      return instrument_pointer -> instument_id;
    }
  }
  qCritical("Cannot find instrument \"%s\"!", qUtf8Printable(instrument_name));
  return -1;
}

auto Song::has_instrument(const QString &maybe_instrument) const -> bool {
  return std::any_of(instrument_pointers.cbegin(), instrument_pointers.cend(),
                     [&maybe_instrument](const auto &instrument_pointer) {
                       return instrument_pointer -> instrument_name == maybe_instrument;
                     });
}
