#pragma once

#include "cell_types/Interval.hpp"
#include "rows/Note.hpp"

struct Player;

static const auto BEND_PER_HALFSTEP = 4096;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_FREQUENCY = 12911.41;  // MIDI 127 plus half step
static const auto QUARTER_STEP = 0.5;
static const auto ZERO_BEND_HALFSTEPS = 2;

[[nodiscard]] auto frequency_to_midi_number(double key) -> double;

[[nodiscard]] auto midi_number_to_frequency(double midi_number) -> double;

struct PitchedNote : Note {
  Interval interval;

  void from_xml(xmlNode& node);

  [[nodiscard]] static auto get_clipboard_schema() -> const char*;

  [[nodiscard]] static auto get_xml_field_name() -> const char*;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char*;

  [[nodiscard]] static auto get_cells_mime() -> const char*;

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool;

  [[nodiscard]] static auto get_pitched() -> const char*;

  // bends channel_number to this note's exact pitch, and returns the nearest
  // MIDI key to play; nullopt (after warning) if the frequency is out of MIDI
  // range, so the caller should abort rather than play a bogus note
  [[nodiscard]] auto get_closest_midi(QWidget& parent, Player& player,
                                      int channel_number, int chord_number,
                                      int note_number) const
      -> std::optional<short>;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant;

  void set_data(int column_number, const QVariant& new_value);

  void column_to_xml(xmlNode& node, int column_number) const;
};
