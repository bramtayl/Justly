#pragma once

class StableIndex {
 public:
  const int chord_index;
  const int note_index;
  const int column_index;
  StableIndex(int chord_index_input, int note_index_input,
              int column_index_input);
};