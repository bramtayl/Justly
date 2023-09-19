#include "Song.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qjsondocument.h>         // for QJsonDocument
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValueRef, QJsonValue

#include <initializer_list>              // for initializer_list
#include <map>               // for operator!=, operator==
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <vector>

#include "notechord/Chord.h"
#include "Instrument.h"  // for Instrument
#include "JsonErrorHandler.h"
#include "TreeNode.h"   // for TreeNode
#include "utilities.h"  // for require_json_field, parse_error

auto Song::get_validator() -> const nlohmann::json_schema::json_validator & {
  const static nlohmann::json_schema::json_validator validator(get_schema());
  return validator;
}

auto Song::get_schema() -> const nlohmann::json& {
  static const nlohmann::json song_schema({
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
    });
  return song_schema;
}

Song::Song(const QString &starting_instrument_input)
    : starting_instrument(starting_instrument_input) {
  if (!Instrument::instrument_exists(starting_instrument_input)) {
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
  try {
    parsed_json = nlohmann::json::parse(song_text.toStdString());
  } catch (const nlohmann::json::parse_error& parse_error) {
    show_parse_error(parse_error);
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

