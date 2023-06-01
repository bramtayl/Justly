#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qnamespace.h>   // for ItemFlags
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"  // for NoteChord
class QString;          // lines 11-11

const auto CHORD_COLUMNS = 8;
const auto CHORD_LEVEL = 1;

class Chord : public NoteChord {
 public:
  ~Chord() override = default;
  Chord(const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
        const QString &default_instrument);
  [[nodiscard]] auto get_level() const -> int override;

  [[nodiscard]] auto flags(int column) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(int column, int role) const -> QVariant override;
  void load(const QJsonObject &json_note_chord) override;
  auto save(QJsonObject &json_map) const -> void override;
  void setData(int column, const QVariant &new_value) override;
  auto copy_pointer() -> std::unique_ptr<NoteChord> override;
  auto get_instrument() -> QString override;
  auto can_set_data(int column, QVariant new_value) -> bool override;
};
