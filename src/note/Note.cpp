#include "note/Note.hpp"

#include <QtGlobal>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "other/other.hpp"
#include "rational/Rational.hpp"
#include "rows/json_field_conversions.hpp"

auto to_note_column(int column) -> NoteColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_NOTE_COLUMNS);
  return static_cast<NoteColumn>(column);
}

Note::Note(const nlohmann::json &json_note) {
  instrument_from_json(*this, json_note);
  interval_from_json(*this, json_note);
  beats_from_json(*this, json_note);
  velocity_ratio_from_json(*this, json_note);

  if (json_note.contains("velocity_ratio")) {
    velocity_ratio = json_to_rational(json_note["velocity_ratio"]);
  }
  if (json_note.contains("words")) {
    words = QString::fromStdString(json_note.value("words", ""));
  }
}

auto Note::get_data(int column_number) const -> QVariant {
  switch (to_note_column(column_number)) {
  case note_instrument_column:
    return QVariant::fromValue(instrument_pointer);
  case note_interval_column:
    return QVariant::fromValue(interval);
  case note_beats_column:
    return QVariant::fromValue(beats);
  case note_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case note_words_column:
    return words;
  }
}

void Note::set_data_directly(int column, const QVariant &new_value) {
  switch (to_note_column(column)) {
  case note_instrument_column:
    instrument_pointer = variant_to_instrument(new_value);
    break;
  case note_interval_column:
    interval = variant_to_interval(new_value);
    break;
  case note_beats_column:
    beats = variant_to_rational(new_value);
    break;
  case note_velocity_ratio_column:
    velocity_ratio = variant_to_rational(new_value);
    break;
  case note_words_column:
    words = variant_to_string(new_value);
    break;
  }
};

void Note::copy_columns_from(const Note &template_row, int left_column,
                             int right_column) {
  for (auto note_column = left_column; note_column <= right_column;
       note_column++) {
    switch (to_note_column(note_column)) {
    case note_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case note_interval_column:
      interval = template_row.interval;
      break;
    case note_beats_column:
      beats = template_row.beats;
      break;
    case note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case note_words_column:
      words = template_row.words;
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
      add_named_to_json(json_note, instrument_pointer, "instrument");
      break;
    case note_interval_column:
      add_interval_to_json(json_note, interval);
      break;
    case note_beats_column:
      add_rational_to_json(json_note, beats, "beats");
      break;
    case note_velocity_ratio_column:
      add_rational_to_json(json_note, velocity_ratio, "velocity_ratio");
      break;
    case note_words_column:
      add_words_to_json(json_note, words);
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
                   {"velocity_ratio", get_rational_schema("velocity ratio")},
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