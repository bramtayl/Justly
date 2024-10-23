#include "note/Note.hpp"

#include <QList>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "note/NotesModel.hpp"
#include "other/other.hpp"
#include "rational/Rational.hpp"

Note::Note(const nlohmann::json &json_note) {
  if (json_note.contains("instrument")) {
    instrument_pointer =
        &json_to_item(get_all_instruments(), json_note["instrument"]);
  };
  if (json_note.contains("interval")) {
    interval = json_to_interval(json_note["interval"]);
  }
  if (json_note.contains("beats")) {
    beats = json_to_rational(json_note["beats"]);
  }
  if (json_note.contains("velocity_ratio")) {
    velocity_ratio = json_to_rational(json_note["velocity_ratio"]);
  }
  if (json_note.contains("tempo_ratio")) {
    tempo_ratio = json_to_rational(json_note["tempo_ratio"]);
  }
  if (json_note.contains("words")) {
    words = QString::fromStdString(json_note.value("words", ""));
  }
}

void Note::copy_columns_from(const Note &template_note, int left_column,
                        int right_column) {
  for (auto note_column = left_column; note_column <= right_column;
       note_column++) {
    switch (to_note_column(note_column)) {
    case note_instrument_column:
      instrument_pointer = template_note.instrument_pointer;
      break;
    case note_interval_column:
      interval = template_note.interval;
      break;
    case note_beats_column:
      beats = template_note.beats;
      break;
    case note_velocity_ratio_column:
      velocity_ratio = template_note.velocity_ratio;
      break;
    case note_tempo_ratio_column:
      tempo_ratio = template_note.tempo_ratio;
      break;
    case note_words_column:
      words = template_note.words;
      break;
    }
  }
};

[[nodiscard]] auto Note::to_json(int left_column,
                                 int right_column) const -> nlohmann::json {
  auto json_note = nlohmann::json::object();
  for (auto note_column = left_column; note_column <= right_column;
       note_column++) {
    switch (to_note_column(note_column)) {
    case note_instrument_column:
      if (instrument_pointer != nullptr) {
        json_note["instrument"] = item_to_json(*instrument_pointer);
      }
      break;
    case note_interval_column:
      if (!interval_is_default(interval)) {
        json_note["interval"] = interval_to_json(interval);
      }
      break;
    case note_beats_column:
      if (!(rational_is_default(beats))) {
        json_note["beats"] = rational_to_json(beats);
      }
      break;
    case note_velocity_ratio_column:
      if (!(rational_is_default(velocity_ratio))) {
        json_note["velocity_ratio"] = rational_to_json(velocity_ratio);
      }
      break;
    case note_tempo_ratio_column:
      if (!(rational_is_default(tempo_ratio))) {
        json_note["tempo_ratio"] = rational_to_json(tempo_ratio);
      }
      break;
    case note_words_column:
      if (!words.isEmpty()) {
        json_note["words"] = words.toStdString().c_str();
      }
      break;
    }
  }
  return json_note;
}

[[nodiscard]] static auto
get_note_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", note_instrument_column},
                         {"maximum", note_words_column}});
}

auto get_notes_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", "the notes"},
       {"items",
        nlohmann::json(
            {{"type", "object"},
             {"description", "a note"},
             {"properties",
              nlohmann::json(
                  {{"instrument", get_instrument_schema()},
                   {"interval", get_interval_schema()},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_percent", get_rational_schema("velocity ratio")},
                   {"tempo_percent", get_rational_schema("tempo ratio")},
                   {"words", get_words_schema()}})}})}});
}

auto get_notes_cells_validator()
    -> const nlohmann::json_schema::json_validator & {
  static const nlohmann::json_schema::json_validator notes_cells_validator =
      make_validator(
          "Notes cells",
          nlohmann::json(
              {{"description", "notes cells"},
               {"type", "object"},
               {"required", {"left_column", "right_column", "notes"}},
               {"properties",
                nlohmann::json({{"left_column",
                                 get_note_column_schema("left note column")},
                                {"right_column",
                                 get_note_column_schema("right note column")},
                                {"notes", get_notes_schema()}})}}));
  return notes_cells_validator;
}