#include "note_chord/Note.hpp"

#include <QtGlobal>
#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "other/templates.hpp"
#include "percussion/Percussion.hpp"

Note::Note() { instrument_pointer = get_instrument_pointer("Marimba"); }

auto notes_to_json(const std::vector<Note> &notes, size_t first_note_number,
                   size_t number_of_notes) -> nlohmann::json {
  nlohmann::json json_notes;
  check_range(notes, first_note_number, number_of_notes);
  std::transform(
      notes.cbegin() + static_cast<int>(first_note_number),
      notes.cbegin() + static_cast<int>(first_note_number + number_of_notes),
      std::back_inserter(json_notes), [](const Note &note) -> nlohmann::json {
        auto json_note = nlohmann::json::object();

        const auto *instrument_pointer = note.instrument_pointer;
        Q_ASSERT(instrument_pointer != nullptr);
        json_note["instrument"] = instrument_pointer->name;

        const auto &interval_or_percussion_pointer =
            note.interval_or_percussion_pointer;
        if (std::holds_alternative<const Percussion *>(
                interval_or_percussion_pointer)) {
          json_note["percussion"] =
              std::get<const Percussion *>(interval_or_percussion_pointer)
                  ->name;
        } else {
          const auto &interval =
              std::get<Interval>(interval_or_percussion_pointer);
          if (!interval_is_default(interval)) {
            json_note["interval"] = interval_to_json(interval);
          }
        }
        const auto &beats = note.beats;
        if (!(rational_is_default(beats))) {
          json_note["beats"] = rational_to_json(beats);
        }
        const auto &velocity_ratio = note.velocity_ratio;
        if (!(rational_is_default(velocity_ratio))) {
          json_note["velocity_ratio"] = rational_to_json(velocity_ratio);
        }
        const auto &tempo_ratio = note.tempo_ratio;
        if (!(rational_is_default(tempo_ratio))) {
          json_note["tempo_ratio"] = rational_to_json(tempo_ratio);
        }
        const auto &words = note.words;
        if (!words.isEmpty()) {
          json_note["words"] = words.toStdString().c_str();
        }
        return json_note;
      });
  return json_notes;
}

void json_to_notes(std::vector<Note> &new_notes,
                   const nlohmann::json &json_notes, size_t number_of_notes) {
  std::transform(
      json_notes.cbegin(),
      json_notes.cbegin() + static_cast<int>(number_of_notes),
      std::back_inserter(new_notes), [](const nlohmann::json &json_note) -> Note {
        Note note;
        if (json_note.contains("instrument")) {
          const auto &instrument_value = json_note["instrument"];
          Q_ASSERT(instrument_value.is_string());
          note.instrument_pointer =
              get_instrument_pointer(instrument_value.get<std::string>());
        }
        if (json_note.contains("interval")) {
          note.interval_or_percussion_pointer =
              json_to_interval(json_note["interval"]);
        }
        if (json_note.contains("percussion")) {
          const auto &percussion_value = json_note["percussion"];
          Q_ASSERT(percussion_value.is_string());
          note.interval_or_percussion_pointer =
              get_percussion_pointer(percussion_value.get<std::string>());
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
