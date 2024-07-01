#pragma once

#include <cstddef>                // for size_t
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "justly/Chord.hpp"  // for Chord
#include "justly/TreeLevel.hpp"
#include "justly/public_constants.hpp"

struct Instrument;
struct NoteChord;

struct JUSTLY_EXPORT Song {
  double starting_key;
  double starting_volume;
  double starting_tempo;
  const Instrument *starting_instrument_pointer;
  std::vector<Chord> chords;

  Song();

  [[nodiscard]] auto get_number_of_children(int parent_number) const -> size_t;
  [[nodiscard]] auto get_chord_number(Chord *chord_pointer) const -> int;
  [[nodiscard]] auto get_note_chord_pointer(int parent_number,
                                            size_t child_number) -> NoteChord *;
  [[nodiscard]] auto get_const_note_chord_pointer(int parent_number,
                                                  size_t child_number) const
      -> const NoteChord *;

  [[nodiscard]] auto json() const -> nlohmann::json;

  void load_starting_values(const nlohmann::json &json_song);
  void load_chords(const nlohmann::json &json_song);

  [[nodiscard]] auto copy(size_t first_child_number, size_t number_of_children,
                          int parent_number) const -> nlohmann::json;
  void insert_directly(size_t first_child_number,
                       const nlohmann::json &json_children, int parent_number);
  void remove_directly(size_t first_child_number, size_t number_of_children,
                       int parent_number);
};

[[nodiscard]] auto JUSTLY_EXPORT verify_children(TreeLevel parent_level,
                                   const nlohmann::json &json_children) -> bool;

[[nodiscard]] auto JUSTLY_EXPORT verify_json_song(const nlohmann::json& json_song) -> bool;