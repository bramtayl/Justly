#include "musicxml/MusicXMLPart.hpp"

#include <QtCore/QTextStream>
#include <QtWidgets/QMessageBox>
#include <array>
#include <numeric>

#include "other/Song.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Row.hpp"
#include "xml/XMLChildren.hpp"

namespace {

auto get_int_or_warn(QWidget& parent, const std::string& content,
                     const QString& title, const QString& message)
    -> std::optional<int> {
  auto maybe_int = string_to_maybe_int(content);
  if (!maybe_int.has_value()) {
    QMessageBox::warning(&parent, title, message);
  }
  return maybe_int;
}

auto get_int_or_warn(QWidget& parent, const xmlNode& element,
                     const QString& title, const QString& message)
    -> std::optional<int> {
  return get_int_or_warn(parent, get_content(element), title, message);
}

auto get_duration(QWidget& parent, xmlNode& measure_element)
    -> std::optional<int> {
  auto& duration_element = get_xml_child(measure_element, "duration");
  if (!xml_content_is_integer(duration_element)) {
    QMessageBox::warning(&parent, QObject::tr("Duration error"),
                         QObject::tr("Fractional durations are not supported"));
    return std::nullopt;
  }
  return get_int_or_warn(parent, duration_element,
                         QObject::tr("Duration error"),
                         QObject::tr("Duration is out of range"));
}

const auto STEPS_PER_OCTAVE = 7;

// the key signature's alteration for each step, indexed C through B
auto get_key_alterations(const int fifths)
    -> std::array<int, STEPS_PER_OCTAVE> {
  // step indices in the order sharps (or, reversed, flats) are added
  static const std::array<int, STEPS_PER_OCTAVE> SHARP_ORDER = {3, 0, 4, 1,
                                                                5, 2, 6};
  std::array<int, STEPS_PER_OCTAVE> alterations = {};
  const auto number_of_accidentals = std::abs(static_cast<long long>(fifths));
  const auto direction = fifths > 0 ? 1 : -1;
  for (auto position = 0; position < STEPS_PER_OCTAVE;
       position = position + 1) {
    const auto order_index =
        fifths > 0 ? position : STEPS_PER_OCTAVE - 1 - position;
    Q_ASSERT(order_index >= 0 && order_index < STEPS_PER_OCTAVE);
    const auto step_index = SHARP_ORDER.at(order_index);
    // past seven accidentals, the cycle wraps around into double accidentals
    const auto times =
        number_of_accidentals > position
            ? (number_of_accidentals - 1 - position) / STEPS_PER_OCTAVE + 1
            : 0;
    Q_ASSERT(step_index >= 0 && step_index < STEPS_PER_OCTAVE);
    alterations.at(step_index) = direction * static_cast<int>(times);
  }
  return alterations;
}

auto parse_attributes(QWidget& parent, xmlNode& attributes_node,
                      MusicXMLPart& part, const int time) -> bool {
  for (auto& attribute_element : get_xml_children(attributes_node)) {
    const auto attribute_name = get_xml_name(attribute_element);
    if (attribute_name == "key") {
      const auto maybe_fifths = get_int_or_warn(
          parent, get_xml_child(attribute_element, "fifths"),
          QObject::tr("Key error"),
          QObject::tr("Fifths value is out of range"));
      if (!maybe_fifths.has_value()) {
        return false;
      }
      part.fifths_changes[time] = maybe_fifths.value();
    } else if (attribute_name == "divisions") {
      if (!xml_content_is_integer(attribute_element)) {
        QMessageBox::warning(
            &parent, QObject::tr("Divisions error"),
            QObject::tr("Fractional divisions are not supported"));
        return false;
      }
      const auto maybe_divisions = get_int_or_warn(
          parent, attribute_element, QObject::tr("Divisions error"),
          QObject::tr("Divisions value is out of range"));
      if (!maybe_divisions.has_value()) {
        return false;
      }
      Q_ASSERT(maybe_divisions.value() > 0);
      part.divisions_changes[time] = maybe_divisions.value();
    } else if (attribute_name == "transpose") {
      auto& chromatic_element = get_xml_child(attribute_element, "chromatic");
      if (!xml_content_is_integer(chromatic_element)) {
        QMessageBox::warning(
            &parent, QObject::tr("Transpose error"),
            QObject::tr("Microtonal transpositions are not supported"));
        return false;
      }
      const auto maybe_chromatic = get_int_or_warn(
          parent, chromatic_element, QObject::tr("Transpose error"),
          QObject::tr("Chromatic value is out of range"));
      if (!maybe_chromatic.has_value()) {
        return false;
      }
      auto octave_change_octaves = 0;
      for (auto& transpose_field : get_xml_children(attribute_element)) {
        if (node_is(transpose_field, "octave-change")) {
          const auto maybe_octave_change = get_int_or_warn(
              parent, transpose_field, QObject::tr("Transpose error"),
              QObject::tr("Octave change value is out of range"));
          if (!maybe_octave_change.has_value()) {
            return false;
          }
          octave_change_octaves = maybe_octave_change.value();
        }
      }
      part.transpose_changes[time] =
          maybe_chromatic.value() +
          octave_change_octaves * HALFSTEPS_PER_OCTAVE;
    }
  }
  return true;
}

// adds the note, unless it's a rest, to the measure, and moves the time
// forward, unless the note is part of the previous chord
auto parse_note(QWidget& parent, xmlNode& note_node, MusicXMLMeasure& measure,
                int& current_time, int& chord_start_time) -> bool {
  // arrows mark Johnston's 7 (down) and el (up)
  static const QMap<std::string, Accidental> accidentals = {
      {"triple-flat", {.chromatic = -3, .septimal_quartertones = 0}},
      {"flat-flat", {.chromatic = -2, .septimal_quartertones = 0}},
      {"flat-flat-down", {.chromatic = -2, .septimal_quartertones = -1}},
      {"flat-flat-up", {.chromatic = -2, .septimal_quartertones = 1}},
      {"flat", {.chromatic = -1, .septimal_quartertones = 0}},
      {"natural-flat", {.chromatic = -1, .septimal_quartertones = 0}},
      {"flat-down", {.chromatic = -1, .septimal_quartertones = -1}},
      {"flat-up", {.chromatic = -1, .septimal_quartertones = 1}},
      {"natural", {.chromatic = 0, .septimal_quartertones = 0}},
      {"natural-down", {.chromatic = 0, .septimal_quartertones = -1}},
      {"natural-up", {.chromatic = 0, .septimal_quartertones = 1}},
      {"sharp", {.chromatic = 1, .septimal_quartertones = 0}},
      {"natural-sharp", {.chromatic = 1, .septimal_quartertones = 0}},
      {"sharp-down", {.chromatic = 1, .septimal_quartertones = -1}},
      {"sharp-up", {.chromatic = 1, .septimal_quartertones = 1}},
      {"double-sharp", {.chromatic = 2, .septimal_quartertones = 0}},
      {"sharp-sharp", {.chromatic = 2, .septimal_quartertones = 0}},
      {"double-sharp-down", {.chromatic = 2, .septimal_quartertones = -1}},
      {"double-sharp-up", {.chromatic = 2, .septimal_quartertones = 1}},
      {"triple-sharp", {.chromatic = 3, .septimal_quartertones = 0}}};

  MusicXMLNote note;
  auto new_chord = true;
  auto is_rest = false;
  for (auto& note_field : get_xml_children(note_node)) {
    const auto& name = get_xml_name(note_field);
    if (name == "pitch") {
      // <alter> is ignored: it can't tell apart e.g. an F raised by an el
      // from an F# lowered by a 7, so pitches are spelled from the
      // accidentals as they would be read
      note.step = get_qstring_content(get_xml_child(note_field, "step"));
      note.octave = xml_to_int(get_xml_child(note_field, "octave"));
    } else if (name == "accidental") {
      const auto accidental_name = get_content(note_field);
      const auto found_accidental = accidentals.find(accidental_name);
      if (found_accidental == accidentals.end()) {
        QMessageBox::warning(&parent, QObject::tr("Pitch error"),
                             QObject::tr("Accidental %1 is not supported")
                                 .arg(QString::fromStdString(accidental_name)));
        return false;
      }
      note.accidental = found_accidental.value();
    } else if (name == "staff") {
      note.staff = get_qstring_content(note_field);
    } else if (name == "duration") {
      if (!xml_content_is_integer(note_field)) {
        QMessageBox::warning(
            &parent, QObject::tr("Note duration error"),
            QObject::tr("Fractional note durations are not supported"));
        return false;
      }
      const auto maybe_duration = get_int_or_warn(
          parent, note_field, QObject::tr("Note duration error"),
          QObject::tr("Note duration is out of range"));
      if (!maybe_duration.has_value()) {
        return false;
      }
      note.duration = maybe_duration.value();
    } else if (name == "unpitched") {
      note.is_pitched = false;
    } else if (name == "tie") {
      const auto tie_type = get_property(note_field, "type");
      if (tie_type == "stop") {
        note.tie_stop = true;
      } else {
        Q_ASSERT(tie_type == "start");
        note.tie_start = true;
      }
    } else if (name == "chord") {
      new_chord = false;
    } else if (name == "rest") {
      is_rest = true;
    } else if (name == "instrument") {
      note.instrument_id = get_property(note_field, "id");
    }
  }

  if (note.duration == 0) {
    QMessageBox::warning(&parent, QObject::tr("Note duration error"),
                         QObject::tr("Notes without durations not supported"));
    return false;
  }
  if (new_chord) {
    chord_start_time = current_time;
    current_time += note.duration;
  }
  if (!is_rest) {
    note.start_time = chord_start_time;
    measure.notes.push_back(std::move(note));
  }
  return true;
}

// records forward/backward repeats and first/second-ending brackets onto the
// measure, so the part can be unrolled later
auto parse_barline(QWidget& parent, xmlNode& barline_node,
                   MusicXMLMeasure& measure, QList<int>& active_ending_numbers)
    -> bool {
  for (auto& child : get_xml_children(barline_node)) {
    if (node_is(child, "repeat")) {
      const auto direction = get_property(child, "direction");
      if (direction == "forward") {
        measure.has_forward_repeat = true;
      } else {
        Q_ASSERT(direction == "backward");
        measure.has_backward_repeat = true;
        auto* const times_property =
            xmlGetProp(&child, c_string_to_xml_string("times"));
        const auto times_text = times_property == nullptr
                                    ? std::string()
                                    : xml_string_to_string(times_property);
        if (!times_text.empty()) {
          const auto maybe_times = get_int_or_warn(
              parent, times_text, QObject::tr("Repeat error"),
              QObject::tr("Repeat times is out of range"));
          if (!maybe_times.has_value()) {
            return false;
          }
          measure.repeat_times = maybe_times.value();
        }
      }
    } else if (node_is(child, "ending")) {
      if (get_property(child, "type") == "start") {
        QList<int> ending_numbers;
        const auto numbers_text =
            QString::fromStdString(get_property(child, "number"));
        for (const auto& token : numbers_text.split(',', Qt::SkipEmptyParts)) {
          bool is_number = false;
          const auto number = token.trimmed().toInt(&is_number);
          if (is_number) {
            ending_numbers.push_back(number);
          }
        }
        for (const auto number : ending_numbers) {
          if (!active_ending_numbers.contains(number)) {
            active_ending_numbers.push_back(number);
          }
          if (!measure.ending_numbers.contains(number)) {
            measure.ending_numbers.push_back(number);
          }
        }
      } else {  // "stop" or "discontinue"
        active_ending_numbers.clear();
      }
    }
  }
  return true;
}

// times are in the part's own divisions, as written
auto parse_part(QWidget& parent, xmlNode& part_node, MusicXMLPart& part)
    -> bool {
  auto current_time = 0;
  auto chord_start_time = current_time;
  QList<int> active_ending_numbers;
  for (auto& measure_node : get_xml_children(part_node)) {
    MusicXMLMeasure measure;
    measure.number = static_cast<int>(part.measures.size()) + 1;
    measure.start_time = current_time;
    measure.ending_numbers = active_ending_numbers;
    for (auto& measure_element : get_xml_children(measure_node)) {
      const auto measure_element_name = get_xml_name(measure_element);
      if (measure_element_name == "attributes") {
        if (!parse_attributes(parent, measure_element, part, current_time)) {
          return false;
        }
      } else if (measure_element_name == "note") {
        if (!parse_note(parent, measure_element, measure, current_time,
                        chord_start_time)) {
          return false;
        }
      } else if (measure_element_name == "backup") {
        const auto duration = get_duration(parent, measure_element);
        if (!duration.has_value()) {
          return false;
        }
        current_time -= duration.value();
        chord_start_time = current_time;
      } else if (measure_element_name == "forward") {
        const auto duration = get_duration(parent, measure_element);
        if (!duration.has_value()) {
          return false;
        }
        current_time += duration.value();
        chord_start_time = current_time;
      } else if (measure_element_name == "barline") {
        if (!parse_barline(parent, measure_element, measure,
                           active_ending_numbers)) {
          return false;
        }
      }
    }
    measure.end_time = current_time;
    part.measures.push_back(std::move(measure));
  }
  return true;
}

}  // namespace

auto get_most_recent(const QMap<int, int>& changes, const int time,
                     const int default_value) -> int {
  auto iterator = changes.upperBound(time);
  if (iterator == changes.cbegin()) {
    return default_value;
  }
  --iterator;
  return iterator.value();
}

auto get_playback_order(const QList<MusicXMLMeasure>& measures) -> QList<int> {
  QList<int> playback_order;
  const auto number_of_measures = static_cast<int>(measures.size());
  auto repeat_start_index = -1;
  auto block_start_index = 0;

  const auto flush = [&](const int first_index, const int last_index) -> auto {
    for (auto index = first_index; index <= last_index; index = index + 1) {
      playback_order.push_back(index);
    }
  };

  auto measure_index = 0;
  while (measure_index < number_of_measures) {
    const auto& measure = measures.at(measure_index);
    if (measure.has_forward_repeat) {
      flush(block_start_index, measure_index - 1);
      repeat_start_index = measure_index;
      block_start_index = measure_index;
    }
    if (measure.has_backward_repeat) {
      const auto start_index =
          repeat_start_index == -1 ? block_start_index : repeat_start_index;
      // a later ending (e.g. the second ending) has no repeat barline of
      // its own; it just continues on directly after the measure with the
      // backward repeat, so absorb any immediately-following ending measures
      auto block_end_index = measure_index;
      while (block_end_index + 1 < number_of_measures &&
             !measures.at(block_end_index + 1).ending_numbers.isEmpty()) {
        block_end_index = block_end_index + 1;
      }
      for (auto pass_number = 1; pass_number <= measure.repeat_times;
           pass_number = pass_number + 1) {
        for (auto inner_index = start_index; inner_index <= block_end_index;
             inner_index = inner_index + 1) {
          const auto& ending_numbers =
              measures.at(inner_index).ending_numbers;
          if (ending_numbers.isEmpty() ||
              ending_numbers.contains(pass_number)) {
            playback_order.push_back(inner_index);
          }
        }
      }
      measure_index = block_end_index;
      block_start_index = block_end_index + 1;
      repeat_start_index = -1;
    }
    measure_index = measure_index + 1;
  }
  flush(block_start_index, number_of_measures - 1);
  return playback_order;
}

auto parse_musicxml(QWidget& parent, xmlNode& score_partwise)
    -> std::optional<QList<MusicXMLPart>> {
  // the part-list comes before the parts
  QMap<std::string, MusicXMLPart> listed_parts;
  QList<MusicXMLPart> parts;
  for (auto& part_node : get_xml_children(score_partwise)) {
    const auto part_node_name = get_xml_name(part_node);
    if (part_node_name == "part-list") {
      for (auto& score_part : get_xml_children(part_node)) {
        if (node_is(score_part, "score-part")) {
          MusicXMLPart part;
          for (auto& field_node : get_xml_children(score_part)) {
            const auto child_name = get_xml_name(field_node);
            if (child_name == "part-name") {
              part.name = get_qstring_content(field_node);
            } else if (child_name == "score-instrument") {
              part.instrument_names[get_property(field_node, "id")] =
                  get_qstring_content(
                      get_xml_child(field_node, "instrument-name"));
            }
          }
          listed_parts[get_property(score_part, "id")] = std::move(part);
        }
      }
    } else if (part_node_name == "part") {
      const auto part_id = get_property(part_node, "id");
      auto part = listed_parts.value(part_id);
      part.id = QString::fromStdString(part_id);
      if (!parse_part(parent, part_node, part)) {
        return std::nullopt;
      }
      parts.push_back(std::move(part));
    }
  }
  return parts;
}

void fill_in_accidentals(MusicXMLPart& part) {
  static const QMap<QString, int> step_indices = {
      {"C", 0}, {"D", 1}, {"E", 2}, {"F", 3}, {"G", 4}, {"A", 5}, {"B", 6}};
  static const std::array<int, STEPS_PER_OCTAVE> step_halfsteps = {
      0, 2, 4, 5, 7, 9, 11};
  for (auto& measure : part.measures) {
    // an accidental lasts until the end of its measure, for notes on the
    // same staff, step, and octave
    QMap<QString, Accidental> measure_accidentals;
    for (auto& note : measure.notes) {
      if (!note.is_pitched) {
        continue;
      }
      // the schema only allows steps A through G
      Q_ASSERT(step_indices.contains(note.step));
      const auto step_index = step_indices.value(note.step);
      const auto accidental_key =
          note.staff + ":" + note.step + ":" + QString::number(note.octave);
      Accidental accidental;
      if (note.accidental.has_value()) {
        accidental = note.accidental.value();
        measure_accidentals[accidental_key] = accidental;
      } else if (measure_accidentals.contains(accidental_key)) {
        accidental = measure_accidentals[accidental_key];
      } else {
        accidental.chromatic =
            get_key_alterations(
                get_most_recent(part.fifths_changes, note.start_time, 0))
                .at(step_index);
      }
      note.midi_number = step_halfsteps.at(step_index) + accidental.chromatic +
                         note.octave * HALFSTEPS_PER_OCTAVE + C_0_MIDI;
      note.septimal_quartertones = accidental.septimal_quartertones;
    }
  }
}

void untranspose(MusicXMLPart& part) {
  // going up a halfstep is going up seven fifths, give or take octaves
  static const auto FIFTHS_PER_HALFSTEP = 7;
  const auto& transpose_changes = part.transpose_changes;
  for (auto& measure : part.measures) {
    for (auto& note : measure.notes) {
      if (note.is_pitched) {
        note.midi_number =
            note.midi_number +
            get_most_recent(transpose_changes, note.start_time, 0);
      }
    }
  }
  // the key changes whenever either the written key or the transposition does
  auto change_times = part.fifths_changes.keys();
  change_times.append(transpose_changes.keys());
  QMap<int, int> fifths_changes;
  for (const auto time : change_times) {
    fifths_changes[time] =
        get_most_recent(part.fifths_changes, time, 0) +
        FIFTHS_PER_HALFSTEP * (get_most_recent(transpose_changes, time, 0) %
                               HALFSTEPS_PER_OCTAVE);
  }
  part.fifths_changes = std::move(fifths_changes);
  part.transpose_changes.clear();
}

void combine_ties(MusicXMLPart& part) {
  // a tie only ever connects notes within the same voice, so an in-progress
  // tie must be looked up by instrument as well as pitch -- otherwise two
  // simultaneous instruments tying the same pitch clobber each other's
  // still-open note. Keyed by written step and octave rather than pitch,
  // since a note tied across a barline usually drops its accidental, but
  // still continues the pitch it was tied from
  QMap<QString, MusicXMLNote*> tied_notes;
  for (auto& measure : part.measures) {
    for (auto& note : measure.notes) {
      const auto tied_note_key = QString::fromStdString(note.instrument_id) +
                                 ":" + note.step + ":" +
                                 QString::number(note.octave);
      const auto tied_notes_iterator = tied_notes.find(tied_note_key);
      if (note.tie_stop && tied_notes_iterator != tied_notes.end()) {
        auto& previous_note = *tied_notes_iterator.value();
        previous_note.duration = previous_note.duration + note.duration;
        if (!note.tie_start) {
          tied_notes.erase(tied_notes_iterator);
        }
      } else {
        // no matching tie-start -- the schema doesn't require ties to be
        // well-formed, so a malformed or hand-edited file can have an orphan
        // tie-stop; treat it as an unstarted note
        note.tie_stop = false;
        if (note.tie_start) {
          tied_notes[tied_note_key] = &note;
        }
      }
    }
  }
  for (auto& measure : part.measures) {
    measure.notes.removeIf(
        [](const MusicXMLNote& note) -> bool { return note.tie_stop; });
  }
}

auto get_song_divisions(const QList<MusicXMLPart>& parts) -> int {
  auto song_divisions = 1;
  for (const auto& part : parts) {
    for (const auto divisions : part.divisions_changes) {
      song_divisions = std::lcm(song_divisions, divisions);
    }
  }
  return song_divisions;
}

namespace {

auto get_song_time(const QMap<int, int>& divisions_changes,
                   const int song_divisions, const int time) -> int {
  auto song_time = 0;
  auto last_change_time = 0;
  auto time_per_division = song_divisions;
  for (const auto [change_time, divisions] :
       divisions_changes.asKeyValueRange()) {
    if (change_time > time) {
      break;
    }
    song_time = song_time + time_per_division * (change_time - last_change_time);
    last_change_time = change_time;
    time_per_division = song_divisions / divisions;
  }
  return song_time + time_per_division * (time - last_change_time);
}

}  // namespace

void normalize_divisions(MusicXMLPart& part, const int song_divisions) {
  const auto& divisions_changes = part.divisions_changes;
  const auto to_song_time = [&divisions_changes,
                             song_divisions](const int time) -> int {
    return get_song_time(divisions_changes, song_divisions, time);
  };
  for (auto& measure : part.measures) {
    for (auto& note : measure.notes) {
      note.duration =
          note.duration *
          (song_divisions /
           get_most_recent(divisions_changes, note.start_time, 1));
      note.start_time = to_song_time(note.start_time);
    }
    measure.start_time = to_song_time(measure.start_time);
    measure.end_time = to_song_time(measure.end_time);
  }
  QMap<int, int> fifths_changes;
  for (const auto [time, fifths] : part.fifths_changes.asKeyValueRange()) {
    fifths_changes[to_song_time(time)] = fifths;
  }
  part.fifths_changes = std::move(fifths_changes);
  part.divisions_changes.clear();
}

void unroll_repeats(MusicXMLPart& part) {
  const auto& fifths_changes = part.fifths_changes;
  QList<MusicXMLMeasure> unrolled_measures;
  QMap<int, int> unrolled_fifths_changes;
  auto time = 0;
  auto previous_index = -1;
  for (const auto measure_index : get_playback_order(part.measures)) {
    auto measure = part.measures.at(measure_index);
    const auto offset = time - measure.start_time;
    if (measure_index != previous_index + 1) {
      // jumping to another part of the score jumps to its key too
      unrolled_fifths_changes[time] =
          get_most_recent(fifths_changes, measure.start_time, 0);
    }
    for (auto iterator = fifths_changes.lowerBound(measure.start_time);
         iterator != fifths_changes.cend() && iterator.key() < measure.end_time;
         ++iterator) {
      unrolled_fifths_changes[iterator.key() + offset] = iterator.value();
    }
    for (auto& note : measure.notes) {
      note.start_time = note.start_time + offset;
    }
    measure.start_time = time;
    measure.end_time = measure.end_time + offset;
    time = measure.end_time;
    previous_index = measure_index;
    unrolled_measures.push_back(std::move(measure));
  }
  part.measures = std::move(unrolled_measures);
  part.fifths_changes = std::move(unrolled_fifths_changes);
}

auto assign_voices(QList<MusicXMLPart>& parts) -> VoiceNames {
  VoiceNames voice_names;
  QMap<QString, int> pitched_voice_numbers;
  QMap<QString, int> unpitched_voice_numbers;
  for (auto& part : parts) {
    for (auto& measure : part.measures) {
      for (auto& note : measure.notes) {
        const auto instrument_name =
            part.instrument_names.value(note.instrument_id);
        QTextStream stream(&note.words);
        stream << QObject::tr("Part ") << part.name;
        if (instrument_name != "") {
          stream << QObject::tr(" instrument ") << instrument_name;
        }
        const auto voice_key =
            part.id + ":" + QString::fromStdString(note.instrument_id);
        auto& voice_numbers =
            note.is_pitched ? pitched_voice_numbers : unpitched_voice_numbers;
        auto& names =
            note.is_pitched ? voice_names.pitched : voice_names.unpitched;
        const auto found_voice_number = voice_numbers.find(voice_key);
        if (found_voice_number != voice_numbers.end()) {
          note.voice_number = found_voice_number.value();
        } else {
          note.voice_number = static_cast<int>(names.size());
          voice_numbers[voice_key] = note.voice_number;
          names.push_back(instrument_name.isEmpty() ? part.name
                                                    : instrument_name);
        }
      }
    }
  }
  return voice_names;
}

auto get_chords(const QList<MusicXMLPart>& parts) -> QMap<int, MusicXMLChord> {
  QMap<int, MusicXMLChord> chords;
  for (const auto& part : parts) {
    for (const auto& measure : part.measures) {
      for (const auto& note : measure.notes) {
        auto& chord = chords[note.start_time];
        (note.is_pitched ? chord.pitched_notes : chord.unpitched_notes)
            .push_back(note);
      }
    }
  }
  return chords;
}

auto get_midi_keys(const QList<MusicXMLPart>& parts) -> QMap<int, int> {
  static const auto FIFTH_HALFSTEPS = 7;
  QMap<int, int> midi_keys;
  for (const auto& part : parts) {
    for (const auto [time, fifths] : part.fifths_changes.asKeyValueRange()) {
      const auto [octave, degree] = get_octave_degree(FIFTH_HALFSTEPS * fifths);
      midi_keys[time] = MIDDLE_C_MIDI + degree;
    }
  }
  return midi_keys;
}

auto get_measure_numbers(const QList<MusicXMLPart>& parts) -> QMap<int, int> {
  QMap<int, int> measure_numbers;
  for (const auto& part : parts) {
    for (const auto& measure : part.measures) {
      measure_numbers[measure.start_time] = measure.number;
    }
  }
  return measure_numbers;
}
