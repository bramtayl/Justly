#pragma once

#include <qjsonvalue.h>  // for QJsonObject

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"  // for NoteChord

class Instrument;
class QString;  // lines 11-11

const auto CHORD_COLUMNS = 8;

class Chord : public NoteChord {
 public:
  Chord();
  ~Chord() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;

  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] auto symbol_for() const -> QString override;

  [[nodiscard]] static auto verify_json(
      const QJsonValue &chord_value, const std::vector<Instrument> &instruments)
      -> bool;
};
