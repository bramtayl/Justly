#include "rows/Chord.hpp"

#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <libxml/parser.h>

#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "other/helpers.hpp"
#include "rows/Row.hpp"
#include "sound/PlayState.hpp"

void Chord::from_xml(xmlNode &node) {
  auto *field_pointer = xmlFirstElementChild(&node);
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "beats") {
      set_rational_from_xml(beats, field_node);
    } else if (name == "velocity_ratio") {
      set_rational_from_xml(velocity_ratio, field_node);
    } else if (name == "tempo_ratio") {
      set_rational_from_xml(tempo_ratio, field_node);
    } else if (name == "words") {
      words = get_qstring_content(field_node);;
    } else if (name == "interval") {
      set_interval_from_xml(interval, field_node);
    } else if (name == "pitched_notes") {
      xml_to_rows(pitched_notes, field_node);
    } else if (name == "unpitched_notes") {
      xml_to_rows(unpitched_notes, field_node);
    } else {
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
}

auto Chord::get_clipboard_schema() -> const char * {
  return "chords_clipboard.xsd";
}

auto Chord::get_xml_field_name() -> const char * {
  return "chord";
}

auto Chord::get_number_of_columns() -> int {
  return static_cast<int>(ChordColumn::number_of_chord_columns);
}

auto Chord::get_column_name(int column_number) -> const char * {
  switch (static_cast<ChordColumn>(column_number)) {
  case ChordColumn::number_of_chord_columns:
    Q_UNREACHABLE();
  case ChordColumn::chord_interval_column:
    return "Interval";
  case ChordColumn::chord_beats_column:
    return "Beats";
  case ChordColumn::chord_velocity_ratio_column:
    return "Velocity ratio";
  case ChordColumn::chord_tempo_ratio_column:
    return "Tempo ratio";
  case ChordColumn::chord_words_column:
    return "Words";
  case ChordColumn::chord_pitched_notes_column:
    return "Pitched notes";
  case ChordColumn::chord_unpitched_notes_column:
    return "Unpitched notes";
  }
  Q_UNREACHABLE();
}

auto Chord::get_cells_mime() -> const char * {
  return "application/prs.chords_cells+xml";
}

auto Chord::is_column_editable(int column_number) -> bool {
  return column_number != static_cast<int>(ChordColumn::chord_pitched_notes_column) &&
         column_number != static_cast<int>(ChordColumn::chord_unpitched_notes_column);
}

auto Chord::get_data(const int column_number) const -> QVariant {
  switch (static_cast<ChordColumn>(column_number)) {
  case ChordColumn::number_of_chord_columns:
    Q_UNREACHABLE();
  case ChordColumn::chord_interval_column:
    return QVariant::fromValue(interval);
  case ChordColumn::chord_beats_column:
    return QVariant::fromValue(beats);
  case ChordColumn::chord_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case ChordColumn::chord_tempo_ratio_column:
    return QVariant::fromValue(tempo_ratio);
  case ChordColumn::chord_words_column:
    return words;
  case ChordColumn::chord_pitched_notes_column:
    return pitched_notes.size();
  case ChordColumn::chord_unpitched_notes_column:
    return unpitched_notes.size();
  }
  Q_UNREACHABLE();
}

void Chord::set_data(const int column_number, const QVariant &new_value) {
  switch (static_cast<ChordColumn>(column_number)) {
  case ChordColumn::number_of_chord_columns:
    Q_UNREACHABLE();
  case ChordColumn::chord_interval_column:
    interval = variant_to<Interval>(new_value);
    break;
  case ChordColumn::chord_beats_column:
    beats = variant_to<Rational>(new_value);
    break;
  case ChordColumn::chord_velocity_ratio_column:
    velocity_ratio = variant_to<Rational>(new_value);
    break;
  case ChordColumn::chord_tempo_ratio_column:
    tempo_ratio = variant_to<Rational>(new_value);
    break;
  case ChordColumn::chord_words_column:
    words = variant_to<QString>(new_value);
    break;
  case ChordColumn::chord_pitched_notes_column:
  case ChordColumn::chord_unpitched_notes_column:
    // not editable; see is_column_editable
    Q_UNREACHABLE();
  }
}

void Chord::copy_column_from(const Chord &template_row, const int column_number) {
  switch (static_cast<ChordColumn>(column_number)) {
  case ChordColumn::number_of_chord_columns:
    Q_UNREACHABLE();
  case ChordColumn::chord_interval_column:
    interval = template_row.interval;
    break;
  case ChordColumn::chord_beats_column:
    beats = template_row.beats;
    break;
  case ChordColumn::chord_velocity_ratio_column:
    velocity_ratio = template_row.velocity_ratio;
    break;
  case ChordColumn::chord_tempo_ratio_column:
    tempo_ratio = template_row.tempo_ratio;
    break;
  case ChordColumn::chord_words_column:
    words = template_row.words;
    break;
  case ChordColumn::chord_pitched_notes_column:
    pitched_notes = template_row.pitched_notes;
    break;
  case ChordColumn::chord_unpitched_notes_column:
    unpitched_notes = template_row.unpitched_notes;
    break;
  }
}

void Chord::column_to_xml(xmlNode &chord_node, const int column_number) const {
  switch (static_cast<ChordColumn>(column_number)) {
  case ChordColumn::number_of_chord_columns:
    Q_UNREACHABLE();
  case ChordColumn::chord_pitched_notes_column:
    maybe_set_xml_rows(chord_node, "pitched_notes", pitched_notes);
    break;
  case ChordColumn::chord_unpitched_notes_column:
    maybe_set_xml_rows(chord_node, "unpitched_notes", unpitched_notes);
    break;
  case ChordColumn::chord_interval_column:
    maybe_add_interval_to_xml(chord_node, "interval", interval);
    break;
  case ChordColumn::chord_beats_column:
    maybe_add_rational_to_xml(chord_node, "beats", beats);
    break;
  case ChordColumn::chord_velocity_ratio_column:
    maybe_add_rational_to_xml(chord_node, "velocity_ratio", velocity_ratio);
    break;
  case ChordColumn::chord_tempo_ratio_column:
    maybe_add_rational_to_xml(chord_node, "tempo_ratio", tempo_ratio);
    break;
  case ChordColumn::chord_words_column:
    maybe_add_qstring_to_xml(chord_node, "words", words);
    break;
  }
}

void Chord::to_xml(xmlNode &chord_node) const {
  maybe_set_xml_rows(chord_node, "pitched_notes", pitched_notes);
  maybe_set_xml_rows(chord_node, "unpitched_notes", unpitched_notes);
  maybe_add_interval_to_xml(chord_node, "interval", interval);
  maybe_add_rational_to_xml(chord_node, "beats", beats);
  maybe_add_rational_to_xml(chord_node, "velocity_ratio", velocity_ratio);
  maybe_add_rational_to_xml(chord_node, "tempo_ratio", tempo_ratio);
  maybe_add_qstring_to_xml(chord_node, "words", words);
}

void modulate(PlayState &play_state, const Chord &chord) {
  play_state.current_key =
      play_state.current_key * interval_to_double(chord.interval);
  play_state.current_velocity =
      play_state.current_velocity * rational_to_double(chord.velocity_ratio);
  play_state.current_tempo =
      play_state.current_tempo * rational_to_double(chord.tempo_ratio);
}

void move_time(PlayState &play_state, const Chord &chord) {
  play_state.current_time =
      play_state.current_time +
      get_duration_in_milliseconds(play_state.current_tempo,
                                   rational_to_double(chord.beats));
}
