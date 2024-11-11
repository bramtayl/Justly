#pragma once

#include <QVariant>
#include <nlohmann/json.hpp>

#include "row/note/Note.hpp"

struct PercussionInstrument;
struct PercussionSet;
struct Player;
struct Program;

struct UnpitchedNote : Note {
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;

  UnpitchedNote() = default;
  explicit UnpitchedNote(const nlohmann::json &json_note);

  [[nodiscard]] auto get_closest_midi(Player &player, int channel_number,
                                      int chord_number,
                                      int note_number) const -> short override;

  [[nodiscard]] auto
  get_program(const Player &player, int chord_number,
              int note_number) const -> const Program & override;

  [[nodiscard]] static auto get_fields_schema() -> nlohmann::json;

  [[nodiscard]] static auto get_note_type() -> const char *;
  [[nodiscard]] static auto get_plural_field_for() -> const char *;
  [[nodiscard]] static auto get_type_name() -> const char *;
  [[nodiscard]] static auto get_plural_description() -> const char *;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char *;
  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data(int column, const QVariant &new_value) override;

  void copy_columns_from(const UnpitchedNote &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto columns_to_json(int left_column, int right_column) const
      -> nlohmann::json override;
};
