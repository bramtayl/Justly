#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qnamespace.h>   // for ItemFlags
#include <qvariant.h>     // for QVariant
#include <memory>         // for unique_ptr

#include "NoteChord.h"    // for NoteChord
#include "Utilities.h"    // for TreeLevel

class QString;          // lines 11-11

const auto NOTE_COLUMNS = 9;

class Note : public NoteChord {
 public:
  ~Note() override = default;
  explicit Note(const QString& default_instrument);
  [[nodiscard]] auto get_level() const -> TreeLevel override;

  [[nodiscard]] auto flags(int column) const -> Qt::ItemFlags override;
  void load(const QJsonObject &json_note_chord) override;
  void save(QJsonObject &json_map) const override;
  [[nodiscard]] auto data(int column, int role) const -> QVariant override;
  auto setData(int column, const QVariant &new_value) -> bool override;
  [[nodiscard]] auto copy_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] auto get_instrument() -> QString override;
  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
};
