#include "rows/UnpitchedNote.hpp"

#include "column_numbers/UnpitchedNoteColumn.hpp"

namespace {

// every unpitched note column is one every note has
auto to_note_field(const int column_number) -> NoteField {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::unpitched_note_voice_name_column:
      return NoteField::voice_name;
    case UnpitchedNoteColumn::unpitched_note_beats_column:
      return NoteField::beats;
    case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
      return NoteField::velocity_ratio;
    case UnpitchedNoteColumn::unpitched_note_words_column:
      return NoteField::words;
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
  }
  Q_UNREACHABLE();
}

}  // namespace

void UnpitchedNote::from_xml(xmlNode& node) {
  for (auto& field_node : get_xml_children(node)) {
    note_field_from_xml(get_xml_name(field_node), field_node);
  }
}

auto UnpitchedNote::get_clipboard_schema() -> const char* {
  return "unpitched_notes_clipboard.xsd";
}

auto UnpitchedNote::get_xml_field_name() -> const char* {
  return "unpitched_note";
}

auto UnpitchedNote::get_number_of_columns() -> int {
  return static_cast<int>(
      UnpitchedNoteColumn::number_of_unpitched_note_columns);
}

auto UnpitchedNote::get_column_name(const int column_number) -> const char* {
  return get_field_name(to_note_field(column_number));
}

auto UnpitchedNote::get_cells_mime() -> const char* {
  return "application/prs.unpitched_notes_cells+xml";
}

auto UnpitchedNote::get_pitched() -> const char* {
  // translated where it's shown
  return QT_TRANSLATE_NOOP("QObject", "unpitched");
}

auto UnpitchedNote::get_data(const int column_number) const -> QVariant {
  return get_field(to_note_field(column_number));
}

void UnpitchedNote::set_data(const int column_number,
                             const QVariant& new_value) {
  set_field(to_note_field(column_number), new_value);
}

void UnpitchedNote::column_to_xml(xmlNode& node,
                                  const int column_number) const {
  field_to_xml(node, to_note_field(column_number));
}
