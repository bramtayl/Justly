#include "Song.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qjsondocument.h>  // for QJsonDocument
#include <qjsonobject.h>    // for QJsonObject
#include <qjsonvalue.h>     // for QJsonValueRef, QJsonValue

#include <algorithm>  // for all_of
#include <initializer_list>          // for initializer_list
#include <map>                       // for operator!=, operator==

#include "Chord.h"
#include "TreeNode.h"        // for TreeNode
#include "Instrument.h"  // for Instrument
#include "JsonErrorHandler.h"
#include "utilities.h"       // for require_json_field, parse_error

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json_fwd.hpp>     // for json

auto Song::get_validator() -> nlohmann::json_schema::json_validator& {
  const auto song_schema = QString(R"(
  {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "Song",
    "description": "A Justly song in JSON format",
    "type": "object",
    "properties": {
      "starting_instrument": {
        "type": "string",
        "description": "the starting instrument",
        "enum": %1
      },
      "starting_key": {
        "type": "integer",
        "description": "the starting key, in Hz",
        "minimum": %2,
        "maximum": %3
      },
      "starting_tempo": {
        "type": "integer",
        "description": "the starting tempo, in bpm",
        "minimum": %4,
        "maximum": %5
      },
      "starting_volume": {
        "type": "integer",
        "description": "the starting volume, from 1 to 100",
        "minimum": %6,
        "maximum": %7
      },
      "chords": {
        "type": "array",
        "description": "a list of chords",
        "items": %8
      }
    },
    "required": [
      "starting_key",
      "starting_tempo",
      "starting_volume",
      "starting_instrument"
    ]
  }
  )")
  .arg(Instrument::get_all_instrument_names())
  .arg(MINIMUM_STARTING_KEY)
  .arg(MAXIMUM_STARTING_KEY)
  .arg(MINIMUM_STARTING_TEMPO)
  .arg(MAXIMUM_STARTING_TEMPO)
  .arg(MINIMUM_STARTING_VOLUME)
  .arg(MAXIMUM_STARTING_VOLUME)
  .arg(Chord::get_schema());
  qInfo("%s", qUtf8Printable(song_schema));
  static nlohmann::json_schema::json_validator validator(nlohmann::json::parse(song_schema.toStdString()));
  return validator;
}

Song::Song(const QString &starting_instrument_input)
    : starting_instrument(starting_instrument_input)  {
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


