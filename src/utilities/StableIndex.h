#pragma once

class StableIndex {
 public:
  int chord_index;
  int note_index;
  int column_index;
  StableIndex(int chord_index_input, int note_index_input,
              int column_index_input);
};
