#pragma once

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>
#include <vector>

#include "justly/Note.h"
#include "justly/NoteChord.h"  // for NoteChord

struct Chord : NoteChord {
  std::vector<std::unique_ptr<Note>> note_pointers;

  Chord() = default;
  explicit Chord(const nlohmann::json &);
  ~Chord() override = default;

  [[nodiscard]] auto symbol_for() const -> std::string override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto children_to_json(int, int) const -> nlohmann::json;
  void insert_empty_chilren(int, int);
  void remove_children(int, int);
  void insert_json_chilren(int, const nlohmann::json &);
};
