#pragma once

#include "rows/PitchedNote.hpp"
#include "rows/UnpitchedNote.hpp"

struct PlayState;

struct Chord {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  void from_xml(xmlNode& node);

  [[nodiscard]] static auto get_clipboard_schema() -> const char*;

  [[nodiscard]] static auto get_xml_field_name() -> const char*;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char*;

  [[nodiscard]] static auto get_cells_mime() -> const char*;

  [[nodiscard]] static auto is_column_editable(int column_number) -> bool;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant;

  void set_data(int column_number, const QVariant& new_value);

  void copy_column_from(const Chord& template_row, int column_number);

  void column_to_xml(xmlNode& chord_node, int column_number) const;
};

void modulate(PlayState& play_state, const Chord& chord);

void move_time(PlayState& play_state, const Chord& chord);

// steps play_state through chords [first_chord_number, end_chord_number),
// calling visit(chord_number, chord) once each chord has modulated
// play_state, before its time moves on
template <typename Visit>
static void walk_chords(PlayState& play_state, const QList<Chord>& chords,
                        const int first_chord_number,
                        const int end_chord_number, Visit visit) {
  for (auto chord_number = first_chord_number; chord_number < end_chord_number;
       chord_number = chord_number + 1) {
    const auto& chord = chords.at(chord_number);
    modulate(play_state, chord);
    visit(chord_number, chord);
    move_time(play_state, chord);
  }
}
