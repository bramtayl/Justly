#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qstring.h>      // for QString
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr

#include "Utilities.h"  // for TreeLevel

const auto DEFAULT_NUMERATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto DEFAULT_OCTAVE = 0;
const auto DEFAULT_BEATS = 1;
const auto DEFAULT_VOLUME_PERCENT = 100.0;
const auto DEFAULT_TEMPO_PERCENT = 100.0;
const auto OCTAVE_RATIO = 2.0;
const auto DEFAULT_WORDS = "";

enum ChordNoteFields {
  symbol_column = 0,
  words_column = 1,
  numerator_column = 2,
  denominator_column = 3,
  octave_column = 4,
  beats_column = 5,
  volume_percent_column = 6,
  tempo_percent_column = 7,
  instrument_column = 8
};

class NoteChord {
 public:
  int numerator = DEFAULT_NUMERATOR;
  int denominator = DEFAULT_DENOMINATOR;
  int octave = DEFAULT_OCTAVE;
  int beats = DEFAULT_BEATS;
  double volume_percent = DEFAULT_VOLUME_PERCENT;
  double tempo_percent = DEFAULT_TEMPO_PERCENT;
  QString words = DEFAULT_WORDS;
  QString instrument = "";
  virtual ~NoteChord() = default;

  virtual auto copy_pointer() -> std::unique_ptr<NoteChord> = 0;
  [[nodiscard]] virtual auto get_level() const -> TreeLevel = 0;
  void load(const QJsonObject &json_note_chord);
  [[nodiscard]] auto data(int column, int role) const -> QVariant;
  [[nodiscard]] auto setData(int column, const QVariant &value) -> bool;
  void save(QJsonObject &json_map) const;
  [[nodiscard]] virtual auto new_child_pointer() -> std::unique_ptr<NoteChord> = 0;
  [[nodiscard]] virtual auto symbol_for() const -> QString = 0;
};
