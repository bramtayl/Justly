#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qstring.h>      // for QString

const auto DEFAULT_NUMERATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto DEFAULT_OCTAVE = 0;

const auto OCTAVE_RATIO = 2.0;

class Interval {
public:
    int numerator = DEFAULT_NUMERATOR;
    int denominator = DEFAULT_DENOMINATOR;
    int octave = DEFAULT_OCTAVE;
    void set_text(const QString& interval_text);
    [[nodiscard]] auto get_text() const -> QString;
    auto save(QJsonObject &json_map) const -> void;
    void load(const QJsonObject &json_interval);
    [[nodiscard]] static auto verify_json(const QJsonObject &json_interval) -> bool;
    [[nodiscard]] auto is_default() const -> bool;
    [[nodiscard]] auto get_ratio() const -> double;
};
