#pragma once

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json

#include "notechord/NoteChord.h"  // for NoteChord

class QString;  // lines 11-11

const auto NOTE_COLUMNS = 9;

class Note : public NoteChord {
 public:
  Note();
  ~Note() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;
  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;

  [[nodiscard]] static auto verify_json_items(const nlohmann::json &notes_json)
      -> bool;
};
