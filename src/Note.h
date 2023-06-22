#pragma once

#include <qjsonobject.h>  // for QJsonObject

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"  // for NoteChord
#include "Utilities.h"  // for TreeLevel

class QString;  // lines 11-11

const auto NOTE_COLUMNS = 9;

class Note : public NoteChord {
 public:
  Note();
  ~Note() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;
  [[nodiscard]] auto symbol_for() const -> QString override;

  [[nodiscard]] auto copy_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;

  [[nodiscard]] static auto verify_json(const QJsonObject &json_note,
                          const std::vector<std::unique_ptr<const QString>>
                              &new_instrument_pointers) -> bool;
};
