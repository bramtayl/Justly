#include "justly/SongIndex.hpp"

auto SongIndex::operator==(const SongIndex& other_index) const -> bool {
  return chord_number == other_index.chord_number &&
         note_number == other_index.note_number &&
         note_chord_field == other_index.note_chord_field;
}