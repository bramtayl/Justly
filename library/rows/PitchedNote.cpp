#include "rows/PitchedNote.hpp"

#include "column_numbers/PitchedNoteColumn.hpp"

namespace {
const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
}  // namespace

auto frequency_to_midi_number(const double key) -> double {
  Q_ASSERT(key > 0);
  return (HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY)) +
         CONCERT_A_MIDI;
}

auto midi_number_to_frequency(const double midi_number) -> double {
  return pow(OCTAVE_RATIO,
             (midi_number - CONCERT_A_MIDI) / HALFSTEPS_PER_OCTAVE) *
         CONCERT_A_FREQUENCY;
}

namespace {

auto is_interval_column(const int column_number) -> bool {
  return column_number ==
         static_cast<int>(PitchedNoteColumn::pitched_note_interval_column);
}

// for every column but the interval column
auto to_note_field(const int column_number) -> NoteField {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
    case PitchedNoteColumn::pitched_note_voice_name_column:
      return NoteField::voice_name;
    case PitchedNoteColumn::pitched_note_beats_column:
      return NoteField::beats;
    case PitchedNoteColumn::pitched_note_velocity_ratio_column:
      return NoteField::velocity_ratio;
    case PitchedNoteColumn::pitched_note_words_column:
      return NoteField::words;
    case PitchedNoteColumn::pitched_note_interval_column:
    case PitchedNoteColumn::number_of_pitched_note_columns:
      Q_UNREACHABLE();
  }
  Q_UNREACHABLE();
}

}  // namespace

void PitchedNote::from_xml(xmlNode& node) {
  for (auto& field_node : get_xml_children(node)) {
    const auto name = get_xml_name(field_node);
    if (name == "interval") {
      set_interval_from_xml(interval, field_node);
    } else {
      note_field_from_xml(name, field_node);
    }
  }
}

auto PitchedNote::get_clipboard_schema() -> const char* {
  return "pitched_notes_clipboard.xsd";
}

auto PitchedNote::get_xml_field_name() -> const char* { return "pitched_note"; }

auto PitchedNote::get_number_of_columns() -> int {
  return static_cast<int>(PitchedNoteColumn::number_of_pitched_note_columns);
}

auto PitchedNote::get_column_name(const int column_number) -> const char* {
  if (is_interval_column(column_number)) {
    return "Interval";
  }
  return get_field_name(to_note_field(column_number));
}

auto PitchedNote::get_cells_mime() -> const char* {
  return "application/prs.pitched_notes_cells+xml";
}

auto PitchedNote::get_pitched() -> const char* {
  // translated where it's shown
  return QT_TRANSLATE_NOOP("QObject", "pitched");
}

auto PitchedNote::get_data(const int column_number) const -> QVariant {
  if (is_interval_column(column_number)) {
    return QVariant::fromValue(interval);
  }
  return get_field(to_note_field(column_number));
}

void PitchedNote::set_data(const int column_number, const QVariant& new_value) {
  if (is_interval_column(column_number)) {
    interval = variant_to<Interval>(new_value);
  } else {
    set_field(to_note_field(column_number), new_value);
  }
}

void PitchedNote::column_to_xml(xmlNode& node, const int column_number) const {
  if (is_interval_column(column_number)) {
    maybe_add_interval_to_xml(node, "interval", interval);
  } else {
    field_to_xml(node, to_note_field(column_number));
  }
}
