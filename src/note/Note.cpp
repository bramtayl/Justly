#include "note/Note.hpp"

#include <QList>
#include <algorithm>
#include <iterator>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "other/other.hpp"
#include "rational/Rational.hpp"

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

auto notes_to_json(const QList<Note> &notes, qsizetype first_note_number,
                   qsizetype number_of_notes, NoteColumn left_column,
                   NoteColumn right_column) -> nlohmann::json {
  nlohmann::json json_notes = nlohmann::json::array();
  std::transform(
      notes.cbegin() + static_cast<int>(first_note_number),
      notes.cbegin() + static_cast<int>(first_note_number + number_of_notes),
      std::back_inserter(json_notes), [left_column, right_column](const Note &note) -> nlohmann::json {
        auto json_note = nlohmann::json::object();
        const auto *instrument_pointer = note.instrument_pointer;
        const auto &interval = note.interval;
        const auto &beats = note.beats;
        const auto &velocity_ratio = note.velocity_ratio;
        const auto &tempo_ratio = note.tempo_ratio;
        const auto &words = note.words;
        for (auto note_column = left_column; note_column <= right_column;
             note_column = static_cast<NoteColumn>(note_column + 1)) {
          switch (note_column) {
          case note_instrument_column:
            if (instrument_pointer != nullptr) {
              json_note["instrument"] =
                  item_to_json(*instrument_pointer);
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
      });
  return json_notes;
}

void partial_json_to_notes(QList<Note> &new_notes,
                           const nlohmann::json &json_notes,
                           size_t number_of_notes) {
  std::transform(
      json_notes.cbegin(),
      json_notes.cbegin() + static_cast<int>(number_of_notes),
      std::back_inserter(new_notes),
      [](const nlohmann::json &json_note) -> Note {
        Note note;
        if (json_note.contains("instrument")) {
          note.instrument_pointer =
              &json_to_item(get_all_instruments(), json_note["instrument"]);
        };
        if (json_note.contains("interval")) {
          note.interval = json_to_interval(json_note["interval"]);
        }
        if (json_note.contains("beats")) {
          note.beats = json_to_rational(json_note["beats"]);
        }
        if (json_note.contains("velocity_ratio")) {
          note.velocity_ratio = json_to_rational(json_note["velocity_ratio"]);
        }
        if (json_note.contains("tempo_ratio")) {
          note.tempo_ratio = json_to_rational(json_note["tempo_ratio"]);
        }
        if (json_note.contains("words")) {
          note.words = QString::fromStdString(json_note.value("words", ""));
        }
        return note;
      });
}

void json_to_notes(QList<Note> &new_notes, const nlohmann::json &json_notes) {
  partial_json_to_notes(new_notes, json_notes, json_notes.size());
}