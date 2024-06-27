#pragma once

struct CellIndex {
  int chord_number;
  int note_number;
  int note_chord_field;

  [[nodiscard]] auto operator==(const CellIndex &other_index) const -> bool;
};
