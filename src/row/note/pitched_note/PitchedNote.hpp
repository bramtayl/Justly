#pragma once

#include <QVariant>
#include <nlohmann/json.hpp>

#include "abstract_rational/interval/Interval.hpp"
#include "row/note/Note.hpp"

struct Instrument;
struct Player;
struct Program;

struct PitchedNote : Note {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;

  PitchedNote() = default;
  explicit PitchedNote(const nlohmann::json &json_note);

  [[nodiscard]] auto get_closest_midi(Player &player, int channel_number,
                                      int chord_number,
                                      int note_number) const -> short override;

  [[nodiscard]] auto
  get_program(const Player &player, int chord_number,
              int note_number) const -> const Program & override;

  [[nodiscard]] static auto get_note_type() -> const char *;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char *;
  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data(int column, const QVariant &new_value) override;

  void copy_columns_from(const PitchedNote &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto columns_to_json(int left_column, int right_column) const
      -> nlohmann::json override;
};