#pragma once

#include <qcolor.h>      // for QColor
#include <qnamespace.h>  // for black, lightGray
#include <qstring.h>     // for QString
#include <qvariant.h>    // for QVariant

#include <gsl/pointers>
#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "metatypes/Instrument.h"
#include "metatypes/Interval.h"  // for Interval

const auto MINIMUM_BEATS = 1;
const auto DEFAULT_BEATS = 1;
const auto MAXIMUM_BEATS = 199;

const auto MINIMUM_VOLUME_PERCENT = 1;
const auto DEFAULT_VOLUME_PERCENT = 100.0;
const auto MAXIMUM_VOLUME_PERCENT = 400;

const auto MINIMUM_TEMPO_PERCENT = 1;
const auto DEFAULT_TEMPO_PERCENT = 100.0;
const auto MAXIMUM_TEMPO_PERCENT = 400;

const auto DEFAULT_WORDS = "";

const auto NOTE_CHORD_COLUMNS = 7;

enum NoteChordField {
  symbol_column = 0,
  words_column = 1,
  interval_column = 2,
  beats_column = 3,
  volume_percent_column = 4,
  tempo_percent_column = 5,
  instrument_column = 6
};

const auto NON_DEFAULT_COLOR = QColor(Qt::black);
const auto DEFAULT_COLOR = QColor(Qt::lightGray);

enum TreeLevel {
  root_level = 0,
  chord_level = 1,
  note_level = 2,
};

class NoteChord {
 protected:
  [[nodiscard]] static auto get_instrument_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_words_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_volume_percent_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_tempo_percent_schema() -> nlohmann::json &;
  [[nodiscard]] static auto get_beats_schema() -> nlohmann::json &;

 public:
  Interval interval = Interval();
  int beats = DEFAULT_BEATS;
  double volume_percent = DEFAULT_VOLUME_PERCENT;
  double tempo_percent = DEFAULT_TEMPO_PERCENT;
  std::string words = DEFAULT_WORDS;
  gsl::not_null<const Instrument *> instrument_pointer =
      &(Instrument::get_instrument_by_name(""));
  NoteChord() = default;
  explicit NoteChord(const nlohmann::json &);
  virtual ~NoteChord() = default;

  [[nodiscard]] auto data(NoteChordField, Qt::ItemDataRole) const
      -> QVariant;
  void setData(NoteChordField, const QVariant &);
  [[nodiscard]] virtual auto to_json() const -> nlohmann::json;
  [[nodiscard]] virtual auto symbol_for() const -> std::string = 0;

 private:
  static auto get_text_color(bool) -> QColor;
};
