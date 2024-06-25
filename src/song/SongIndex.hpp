#pragma once

struct SongIndex {
  int chord_number;
  int note_number;
  int note_chord_field;

  [[nodiscard]] auto operator==(const SongIndex &other_index) const -> bool;
};
