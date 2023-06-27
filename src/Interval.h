#pragma once

#include <qregularexpression.h>  // for QRegularExpression
#include <qstring.h>      // for QString

const auto DEFAULT_NUMERATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto DEFAULT_OCTAVE = 0;

const auto OCTAVE_RATIO = 2.0;

const auto INTERVAL_PATTERN = QRegularExpression(
      R"((?<numerator>\d+)(\/(?<denominator>\d+))?(o(?<octave>-?\d+))?)");

class Interval {
public:
    int numerator = DEFAULT_NUMERATOR;
    int denominator = DEFAULT_DENOMINATOR;
    int octave = DEFAULT_OCTAVE;
    void set_text(const QString& interval_text);
    [[nodiscard]] auto get_text() const -> QString;
    [[nodiscard]] static auto verify_json(const QString &interval_text) -> bool;
    [[nodiscard]] auto is_default() const -> bool;
    [[nodiscard]] auto get_ratio() const -> double;
};
