#pragma once

#include <memory>  // for unique_ptr

#include <nlohmann/json_fwd.hpp>  // for json

#include "NoteChord.h"  // for NoteChord

namespace nlohmann::json_schema {
class json_validator;
}

class QString;  // lines 11-11

const auto NOTE_COLUMNS = 9;

class Note : public NoteChord {
 public:
  Note();
  ~Note() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;
  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] static auto get_schema() -> nlohmann::json&;

  [[nodiscard]] static auto verify_json_items(const QString &note_text) -> bool;
  [[nodiscard]] static auto get_list_validator()
      -> nlohmann::json_schema::json_validator &;
};
