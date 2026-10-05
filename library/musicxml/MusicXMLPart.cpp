#include "musicxml/MusicXMLPart.hpp"

#include <QtCore/QSet>
#include <QtCore/QTextStream>
#include <QtWidgets/QMessageBox>
#include <algorithm>
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

// for xs:decimal fields, which could hold a fraction Justly can't support
auto get_whole_int_or_warn(QWidget& parent, const xmlNode& element,
                           const QString& title,
                           const QString& fractional_message,
                           const QString& range_message) -> std::optional<int> {
  if (!xml_content_is_integer(element)) {
    QMessageBox::warning(&parent, title, fractional_message);
    return std::nullopt;
  }
  return get_int_or_warn(parent, element, title, range_message);
}

auto get_duration(QWidget& parent, xmlNode& measure_element)
    -> std::optional<int> {
  return get_whole_int_or_warn(
      parent, get_xml_child(measure_element, "duration"),
      QObject::tr("Duration error"),
      QObject::tr("Fractional durations are not supported"),
      QObject::tr("Duration is out of range"));
}

// skips anything that isn't a number
auto parse_number_list(const std::string& text) -> QList<int> {
  QList<int> numbers;
  for (const auto& token :
       QString::fromStdString(text).split(',', Qt::SkipEmptyParts)) {
    bool is_number = false;
    const auto number = token.trimmed().toInt(&is_number);
    if (is_number) {
      numbers.push_back(number);
    }
  }
  return numbers;
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
      const auto maybe_fifths =
          get_int_or_warn(parent, get_xml_child(attribute_element, "fifths"),
                          QObject::tr("Key error"),
                          QObject::tr("Fifths value is out of range"));
      if (!maybe_fifths.has_value()) {
        return false;
      }
      part.fifths_changes[time] = maybe_fifths.value();
    } else if (attribute_name == "divisions") {
      const auto maybe_divisions = get_whole_int_or_warn(
          parent, attribute_element, QObject::tr("Divisions error"),
          QObject::tr("Fractional divisions are not supported"),
          QObject::tr("Divisions value is out of range"));
      if (!maybe_divisions.has_value()) {
        return false;
      }
      Q_ASSERT(maybe_divisions.value() > 0);
      part.divisions_changes[time] = maybe_divisions.value();
    } else if (attribute_name == "transpose") {
      const auto maybe_chromatic = get_whole_int_or_warn(
          parent, get_xml_child(attribute_element, "chromatic"),
          QObject::tr("Transpose error"),
          QObject::tr("Microtonal transpositions are not supported"),
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
auto parse_note(QWidget& parent, xmlNode& note_node, const MusicXMLPart& part,
                MusicXMLMeasure& measure, int& current_time,
                int& chord_start_time) -> bool {
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
      const auto maybe_duration = get_whole_int_or_warn(
          parent, note_field, QObject::tr("Note duration error"),
          QObject::tr("Fractional note durations are not supported"),
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
      // the schema only checks that the id is declared somewhere
      if (!part.instrument_names.contains(note.instrument_id)) {
        QMessageBox::warning(
            &parent, QObject::tr("Instrument error"),
            QObject::tr("Instrument %1 in measure %2 isn't in part %3")
                .arg(QString::fromStdString(note.instrument_id))
                .arg(measure.number)
                .arg(part.name));
        return false;
      }
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

void maybe_add_marker(xmlNode& node, const char* name,
                      QList<QString>& markers) {
  const auto maybe_marker = maybe_get_property(node, name);
  if (maybe_marker.has_value()) {
    markers.push_back(QString::fromStdString(maybe_marker.value()));
  }
}

// records forward/backward repeats, first/second-ending brackets, and segnos
// and codas onto the measure, so the part can be unrolled later
auto parse_barline(QWidget& parent, xmlNode& barline_node,
                   MusicXMLMeasure& measure, QList<int>& active_ending_numbers,
                   bool& in_ending) -> bool {
  maybe_add_marker(barline_node, "segno", measure.segnos);
  maybe_add_marker(barline_node, "coda", measure.codas);
  for (auto& child : get_xml_children(barline_node)) {
    if (node_is(child, "repeat")) {
      const auto direction = get_property(child, "direction");
      if (direction == "forward") {
        measure.has_forward_repeat = true;
      } else {
        Q_ASSERT(direction == "backward");
        measure.has_backward_repeat = true;
        const auto times_text = maybe_get_property(child, "times").value_or("");
        if (!times_text.empty()) {
          const auto maybe_times =
              get_int_or_warn(parent, times_text, QObject::tr("Repeat error"),
                              QObject::tr("Repeat times is out of range"));
          if (!maybe_times.has_value()) {
            return false;
          }
          measure.repeat_times = maybe_times.value();
        }
      }
    } else if (node_is(child, "ending")) {
      const auto is_start = get_property(child, "type") == "start";
      // a blank ending has no numbers, so whether an ending is open has to
      // be tracked separately
      if (is_start == in_ending) {
        QMessageBox::warning(
            &parent, QObject::tr("Ending error"),
            (is_start
                 ? QObject::tr("Ending in measure %1 starts before the "
                               "previous ending stops")
                 : QObject::tr("Ending in measure %1 stops without starting"))
                .arg(measure.number));
        return false;
      }
      in_ending = is_start;
      if (is_start) {
        for (const auto number :
             parse_number_list(get_property(child, "number"))) {
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

// records jumps, and the segnos and codas they jump to, onto the measure
void parse_sound(xmlNode& sound_node, MusicXMLMeasure& measure) {
  const auto times = parse_number_list(
      maybe_get_property(sound_node, "time-only").value_or(""));
  const auto maybe_add_jump = [&sound_node, &measure, &times](
                                  const JumpType type,
                                  const char* name) -> void {
    const auto maybe_target = maybe_get_property(sound_node, name);
    // dacapo and forward-repeat can be "no"
    if (maybe_target.has_value() && maybe_target.value() != "no") {
      measure.jumps.push_back(
          {.type = type,
           .target = QString::fromStdString(maybe_target.value()),
           .times = times});
    }
  };
  maybe_add_jump(JumpType::da_capo, "dacapo");
  maybe_add_jump(JumpType::dal_segno, "dalsegno");
  maybe_add_jump(JumpType::to_coda, "tocoda");
  maybe_add_jump(JumpType::fine, "fine");
  maybe_add_marker(sound_node, "segno", measure.segnos);
  maybe_add_marker(sound_node, "coda", measure.codas);
  // a forward repeat that isn't drawn, e.g. at the start of a trio
  if (maybe_get_property(sound_node, "forward-repeat") == "yes") {
    measure.has_forward_repeat = true;
  }
}

// times are in the part's own divisions, as written
auto parse_part(QWidget& parent, xmlNode& part_node, MusicXMLPart& part)
    -> bool {
  auto current_time = 0;
  auto chord_start_time = current_time;
  QList<int> active_ending_numbers;
  auto in_ending = false;
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
        if (!parse_note(parent, measure_element, part, measure, current_time,
                        chord_start_time)) {
          return false;
        }
      } else if (measure_element_name == "backup") {
        const auto duration = get_duration(parent, measure_element);
        if (!duration.has_value()) {
          return false;
        }
        current_time -= duration.value();
        if (current_time < measure.start_time) {
          QMessageBox::warning(
              &parent, QObject::tr("Duration error"),
              QObject::tr("Backup in measure %1 goes back past the start of "
                          "the measure")
                  .arg(measure.number));
          return false;
        }
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
                           active_ending_numbers, in_ending)) {
          return false;
        }
      } else if (measure_element_name == "sound") {
        parse_sound(measure_element, measure);
      } else if (measure_element_name == "direction") {
        auto* const sound_pointer =
            maybe_get_xml_child(measure_element, "sound");
        if (sound_pointer != nullptr) {
          parse_sound(*sound_pointer, measure);
        }
      }
    }
    measure.end_time = current_time;
    part.measures.push_back(std::move(measure));
  }
  if (in_ending) {
    QMessageBox::warning(
        &parent, QObject::tr("Ending error"),
        QObject::tr("Ending in part %1 never stops").arg(part.name));
    return false;
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

namespace {

// the measure marked with the named segno or coda, or, failing that, the
// nearest marked measure in the direction of the jump
auto find_marker(const QList<MusicXMLMeasure>& measures,
                 QList<QString> MusicXMLMeasure::* markers, const QString& name,
                 const int fallback_start_index, const int fallback_step)
    -> std::optional<int> {
  const auto number_of_measures = static_cast<int>(measures.size());
  for (auto index = 0; index < number_of_measures; index = index + 1) {
    if ((measures.at(index).*markers).contains(name)) {
      return index;
    }
  }
  for (auto index = fallback_start_index;
       index >= 0 && index < number_of_measures;
       index = index + fallback_step) {
    if (!(measures.at(index).*markers).isEmpty()) {
      return index;
    }
  }
  return std::nullopt;
}

// whether the ending measure belongs to the last of its set of endings
auto is_in_last_ending(const QList<MusicXMLMeasure>& measures,
                       const int measure_index) -> bool {
  const auto number_of_measures = static_cast<int>(measures.size());
  auto last_index = measure_index;
  while (last_index + 1 < number_of_measures &&
         !measures.at(last_index + 1).ending_numbers.isEmpty()) {
    last_index = last_index + 1;
  }
  const auto& last_ending_numbers = measures.at(last_index).ending_numbers;
  return std::ranges::any_of(measures.at(measure_index).ending_numbers,
                             [&last_ending_numbers](const int number) -> bool {
                               return last_ending_numbers.contains(number);
                             });
}

}  // namespace

auto get_playback_order(const QList<MusicXMLMeasure>& measures) -> QList<int> {
  const auto number_of_measures = static_cast<int>(measures.size());
  QList<int> playback_order;
  QList<int> times_played(number_of_measures, 0);
  // how many times each backward repeat has gone back, this time through
  QList<int> times_repeated(number_of_measures, 0);
  // measures whose da capo or dal segno has been taken
  QSet<int> jumped_from;
  // after a da capo or dal segno, repeats are skipped, and only the last
  // ending is played
  auto after_jump = false;
  // a backward repeat without a forward repeat goes back to the end of the
  // last repeat or set of endings
  auto repeat_start_index = 0;
  auto pass_number = 1;
  auto in_endings = false;
  auto repeating = false;

  auto measure_index = 0;
  while (measure_index < number_of_measures) {
    const auto& measure = measures.at(measure_index);
    const auto& ending_numbers = measure.ending_numbers;
    if (!repeating && (measure.has_forward_repeat ||
                       (in_endings && ending_numbers.isEmpty()))) {
      repeat_start_index = measure_index;
      pass_number = 1;
    }
    repeating = false;
    in_endings = !ending_numbers.isEmpty();
    if (in_endings && !(after_jump ? is_in_last_ending(measures, measure_index)
                                   : ending_numbers.contains(pass_number))) {
      measure_index = measure_index + 1;
      continue;
    }

    playback_order.push_back(measure_index);
    auto& time_through = times_played[measure_index];
    time_through = time_through + 1;
    const auto applies = [time_through](const MusicXMLJump& jump,
                                        const bool by_default) -> bool {
      return jump.times.isEmpty() ? by_default
                                  : jump.times.contains(time_through);
    };

    if (std::ranges::any_of(
            measure.jumps,
            [&applies, after_jump](const MusicXMLJump& jump) -> bool {
              return jump.type == JumpType::fine && applies(jump, after_jump);
            })) {
      break;
    }

    if (measure.has_backward_repeat && !after_jump) {
      auto& repeats = times_repeated[measure_index];
      if (repeats < measure.repeat_times - 1) {
        repeats = repeats + 1;
        pass_number = repeats + 1;
        measure_index = repeat_start_index;
        repeating = true;
        continue;
      }
      repeats = 0;
      if (!in_endings) {
        repeat_start_index = measure_index + 1;
        pass_number = 1;
      }
    }

    std::optional<int> maybe_jump_index;
    for (const auto& jump : measure.jumps) {
      if (jump.type == JumpType::to_coda && applies(jump, after_jump)) {
        maybe_jump_index = find_marker(measures, &MusicXMLMeasure::codas,
                                       jump.target, measure_index + 1, 1);
      } else if ((jump.type == JumpType::da_capo ||
                  jump.type == JumpType::dal_segno) &&
                 applies(jump, !jumped_from.contains(measure_index))) {
        maybe_jump_index = jump.type == JumpType::da_capo
                               ? 0
                               : find_marker(measures, &MusicXMLMeasure::segnos,
                                             jump.target, measure_index, -1);
        if (maybe_jump_index.has_value()) {
          jumped_from.insert(measure_index);
          after_jump = true;
        }
      }
      if (maybe_jump_index.has_value()) {
        break;
      }
    }
    if (maybe_jump_index.has_value()) {
      measure_index = maybe_jump_index.value();
      repeat_start_index = measure_index;
      pass_number = 1;
      in_endings = false;
    } else {
      measure_index = measure_index + 1;
    }
  }
  return playback_order;
}

auto check_navigation(QWidget& parent, const QList<MusicXMLMeasure>& measures)
    -> bool {
  const auto number_of_measures = static_cast<int>(measures.size());
  // a backward repeat without a forward repeat is fine, but not the reverse
  std::optional<int> maybe_open_repeat_index;
  const auto warn_dangling_repeat = [&parent](const int measure_index) -> void {
    QMessageBox::warning(
        &parent, QObject::tr("Repeat error"),
        QObject::tr("Forward repeat in measure %1 has no backward repeat")
            .arg(measure_index + 1));
  };
  for (auto measure_index = 0; measure_index < number_of_measures;
       measure_index = measure_index + 1) {
    const auto& measure = measures.at(measure_index);
    if (measure.has_forward_repeat) {
      if (maybe_open_repeat_index.has_value()) {
        warn_dangling_repeat(maybe_open_repeat_index.value());
        return false;
      }
      maybe_open_repeat_index = measure_index;
    }
    if (measure.has_backward_repeat) {
      maybe_open_repeat_index.reset();
    }
    for (const auto& jump : measure.jumps) {
      if (jump.type == JumpType::dal_segno &&
          !find_marker(measures, &MusicXMLMeasure::segnos, jump.target,
                       measure_index, -1)
               .has_value()) {
        QMessageBox::warning(
            &parent, QObject::tr("Jump error"),
            QObject::tr("Dal segno in measure %1 has no segno to go back to")
                .arg(measure_index + 1));
        return false;
      }
      if (jump.type == JumpType::to_coda &&
          !find_marker(measures, &MusicXMLMeasure::codas, jump.target,
                       measure_index + 1, 1)
               .has_value()) {
        QMessageBox::warning(
            &parent, QObject::tr("Jump error"),
            QObject::tr("To coda in measure %1 has no coda to go to")
                .arg(measure_index + 1));
        return false;
      }
    }
  }
  if (maybe_open_repeat_index.has_value()) {
    warn_dangling_repeat(maybe_open_repeat_index.value());
    return false;
  }
  return true;
}

auto get_score_measures(const QList<MusicXMLPart>& parts)
    -> QList<MusicXMLMeasure> {
  QList<MusicXMLMeasure> score_measures;
  // parse_musicxml checks that every part has the same measures
  if (!parts.empty()) {
    score_measures.resize(parts.at(0).measures.size());
  }
  for (const auto& part : parts) {
    const auto& measures = part.measures;
    for (auto index = 0; index < measures.size(); index = index + 1) {
      auto& score_measure = score_measures[index];
      const auto& measure = measures.at(index);
      score_measure.has_forward_repeat =
          score_measure.has_forward_repeat || measure.has_forward_repeat;
      if (measure.has_backward_repeat && !score_measure.has_backward_repeat) {
        score_measure.has_backward_repeat = true;
        score_measure.repeat_times = measure.repeat_times;
      }
      for (const auto number : measure.ending_numbers) {
        if (!score_measure.ending_numbers.contains(number)) {
          score_measure.ending_numbers.push_back(number);
        }
      }
      score_measure.segnos.append(measure.segnos);
      score_measure.codas.append(measure.codas);
      score_measure.jumps.append(measure.jumps);
    }
  }
  return score_measures;
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
  // the schema can't check this, but the spec requires it
  for (const auto& part : parts) {
    const auto& first_part = parts.at(0);
    if (part.measures.size() != first_part.measures.size()) {
      QMessageBox::warning(
          &parent, QObject::tr("Measure error"),
          QObject::tr("Part %1 has %2 measure(s), but part %3 has %4")
              .arg(part.name)
              .arg(part.measures.size())
              .arg(first_part.name)
              .arg(first_part.measures.size()));
      return std::nullopt;
    }
  }
  return parts;
}

void fill_in_accidentals(MusicXMLPart& part) {
  static const QMap<QString, int> step_indices = {
      {"C", 0}, {"D", 1}, {"E", 2}, {"F", 3}, {"G", 4}, {"A", 5}, {"B", 6}};
  static const std::array<int, STEPS_PER_OCTAVE> step_halfsteps = {0, 2, 4, 5,
                                                                   7, 9, 11};
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

auto combine_ties(QWidget& parent, MusicXMLPart& part) -> bool {
  // a tie only ever connects notes within the same voice, so an in-progress
  // tie must be looked up by instrument as well as pitch -- otherwise two
  // simultaneous instruments tying the same pitch clobber each other's
  // still-open note. Keyed by written step and octave rather than pitch,
  // since a note tied across a barline usually drops its accidental, but
  // still continues the pitch it was tied from
  struct TiedNote {
    MusicXMLNote* note_pointer = nullptr;
    int measure_number = 1;
  };
  QMap<QString, TiedNote> tied_notes;
  // the schema doesn't require ties to be well-formed
  const auto warn_unstopped = [&parent, &part](const int measure_number) {
    QMessageBox::warning(&parent, QObject::tr("Tie error"),
                         QObject::tr("Tie in measure %1 of part %2 never stops")
                             .arg(measure_number)
                             .arg(part.name));
  };
  for (auto& measure : part.measures) {
    for (auto& note : measure.notes) {
      const auto tied_note_key = QString::fromStdString(note.instrument_id) +
                                 ":" + note.step + ":" +
                                 QString::number(note.octave);
      const auto tied_notes_iterator = tied_notes.find(tied_note_key);
      if (note.tie_stop) {
        if (tied_notes_iterator == tied_notes.end()) {
          QMessageBox::warning(
              &parent, QObject::tr("Tie error"),
              QObject::tr("Tie in measure %1 of part %2 never starts")
                  .arg(measure.number)
                  .arg(part.name));
          return false;
        }
        auto& previous_note =
            get_reference(tied_notes_iterator.value().note_pointer);
        previous_note.duration = previous_note.duration + note.duration;
        if (!note.tie_start) {
          tied_notes.erase(tied_notes_iterator);
        }
      } else if (note.tie_start) {
        if (tied_notes_iterator != tied_notes.end()) {
          warn_unstopped(tied_notes_iterator.value().measure_number);
          return false;
        }
        tied_notes[tied_note_key] = {.note_pointer = &note,
                                     .measure_number = measure.number};
      }
    }
  }
  if (!tied_notes.isEmpty()) {
    warn_unstopped(tied_notes.first().measure_number);
    return false;
  }
  for (auto& measure : part.measures) {
    measure.notes.removeIf(
        [](const MusicXMLNote& note) -> bool { return note.tie_stop; });
  }
  return true;
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
    song_time =
        song_time + time_per_division * (change_time - last_change_time);
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
      note.duration = note.duration *
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

void unroll_repeats(MusicXMLPart& part, const QList<int>& playback_order) {
  const auto& fifths_changes = part.fifths_changes;
  QList<MusicXMLMeasure> unrolled_measures;
  QMap<int, int> unrolled_fifths_changes;
  auto time = 0;
  auto previous_index = -1;
  for (const auto measure_index : playback_order) {
    // parse_musicxml checks that every part has the same measures
    Q_ASSERT(measure_index < part.measures.size());
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
