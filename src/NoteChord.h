#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qnamespace.h>   // for ItemFlags
#include <qstring.h>      // for QString
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr

const int DEFAULT_NUMERATOR = 1;
const int DEFAULT_DENOMINATOR = 1;
const int DEFAULT_OCTAVE = 0;
const int DEFAULT_BEATS = 1;
const auto DEFAULT_VOLUME_RATIO = 1.0F;
const auto DEFAULT_TEMPO_RATIO = 1.0F;
const auto OCTAVE_RATIO = 2.0F;
const auto DEFAULT_INSTRUMENT = "STKPlucked";

enum ChordNoteFields {
  symbol_column = 0,
  words_column = 1,
  numerator_column = 2,
  denominator_column = 3,
  octave_column = 4,
  beats_column = 5,
  volume_ratio_column = 6,
  tempo_ratio_column = 7,
  instrument_column = 8
};

class NoteChord {
 public:
  int numerator = DEFAULT_NUMERATOR;
  int denominator = DEFAULT_DENOMINATOR;
  int octave = DEFAULT_OCTAVE;
  int beats = DEFAULT_BEATS;
  double volume_ratio = DEFAULT_VOLUME_RATIO;
  double tempo_ratio = DEFAULT_TEMPO_RATIO;
  QString words;
  QString instrument = DEFAULT_INSTRUMENT;

  virtual ~NoteChord() = default;

  virtual auto copy_pointer() -> std::unique_ptr<NoteChord> = 0;

  static auto error_column(int column) -> void;
  [[nodiscard]] virtual auto flags(int column) const -> Qt::ItemFlags = 0;
  [[nodiscard]] virtual auto get_level() const -> int = 0;
  virtual void load(const QJsonObject &json_note_chord) = 0;
  [[nodiscard]] virtual auto data(int column, int role) const -> QVariant = 0;
  virtual auto setData(int column, const QVariant &value, int role) -> bool = 0;
  virtual auto save(QJsonObject &json_map) const -> void = 0;
  static auto get_double(const QJsonObject &object, const QString &name,
                         double a_default) -> double;
  static auto get_positive_double(const QJsonObject &object,
                                  const QString &name, double a_default)
      -> double;
  static auto get_int(const QJsonObject &object, const QString &name,
                      int a_default) -> int;
  static auto get_positive_int(const QJsonObject &object, const QString &name,
                               int a_default) -> int;
  static auto get_non_negative_int(const QJsonObject &object,
                                   const QString &name, int a_default) -> int;
  static auto get_string(const QJsonObject &object, const QString &name,
                         const QString &a_default = "") -> QString;
};
