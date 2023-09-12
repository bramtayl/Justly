#pragma once

#include <qjsonobject.h>
#include <qmetatype.h>  // for qRegisterMetaType, qRegisterNormaliz...
#include <qstring.h>    // for QString

const auto MINIMUM_NUMERATOR = 1;
const auto DEFAULT_NUMERATOR = 1;
const auto MAXIMUM_NUMERATOR = 199;

const auto MINIMUM_DENOMINATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto MAXIMUM_DENOMINATOR = 199;

const auto MINIMUM_OCTAVE = -9;
const auto DEFAULT_OCTAVE = 0;
const auto MAXIMUM_OCTAVE = 9;

const auto OCTAVE_RATIO = 2.0;

class QRegularExpression;

class Interval {
 public:
  int numerator;
  int denominator;
  int octave;
  static const QRegularExpression interval_pattern;
  explicit Interval(int numerator = DEFAULT_NUMERATOR,
                    int denominator = DEFAULT_DENOMINATOR,
                    int octave = DEFAULT_OCTAVE);
  [[nodiscard]] auto get_text() const -> QString;
  [[nodiscard]] static auto get_pattern() -> QRegularExpression&;
  [[nodiscard]] static auto get_schema() -> QString&;
  [[nodiscard]] auto is_default() const -> bool;
  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] static auto parse_interval(const QString& interval_text)
      -> Interval;
  auto operator==(const Interval& other_interval) const -> bool;
  auto save_to(QJsonObject &json_map) const -> void;
  void load_from(const QJsonObject &json_interval);
};


Q_DECLARE_METATYPE(Interval)