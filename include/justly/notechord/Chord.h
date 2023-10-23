#pragma once

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>
#include <vector>

#include "justly/notechord/Note.h"
#include "justly/notechord/NoteChord.h"  // for NoteChord

class Chord : public NoteChord {
 public:
  std::vector<std::unique_ptr<Note>> note_pointers;

  Chord() = default;
  explicit Chord(const nlohmann::json &);
  ~Chord() override = default;

  [[nodiscard]] auto symbol_for() const -> std::string override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto notes_to_json(int, int) const -> nlohmann::json;
  void insert_empty_notes(int, int);
  void remove_notes(int, int);
  void insert_json_notes(int, const nlohmann::json &);
};
