#pragma once

#include <qcolor.h>       // for QColor
#include <qjsonobject.h>  // for QJsonObject
#include <qjsonvalue.h>   // for QJsonValue
#include <qnamespace.h>   // for black, lightGray
#include <qstring.h>      // for QString
#include <stddef.h>       // for size_t
#include <QColor>
#include <memory>         // for unique_ptr
#include <vector>         // for vector

class QComboBox;   // lines 10-10

enum TreeLevel {
  root_level = 0,
  chord_level = 1,
  note_level = 2,
};

const auto MINIMUM_NUMERATOR = -99;
const auto MAXIMUM_NUMERATOR = 99;
const auto MINIMUM_DENOMINATOR = -99;
const auto MAXIMUM_DENOMINATOR = 99;
const auto MINIMUM_OCTAVE = -99;
const auto MAXIMUM_OCTAVE = 99;
const auto MINIMUM_BEATS = 0;
const auto MAXIMUM_BEATS = 99;
const auto MINIMUM_VOLUME_PERCENT = 1;
const auto MAXIMUM_VOLUME_PERCENT = 200;
const auto MINIMUM_TEMPO_PERCENT = 1;
const auto MAXIMUM_TEMPO_PERCENT = 200;

const auto PERCENT = 100;

const auto NON_DEFAULT_COLOR = QColor(Qt::lightGray);
const auto DEFAULT_COLOR = QColor(Qt::black);

[[nodiscard]] auto get_json_double(const QJsonObject &object, const QString &field_name,
                     double a_default) -> double;
[[nodiscard]] auto get_json_int(const QJsonObject &object, const QString &field_name,
                  int a_default) -> int;
[[nodiscard]] auto get_json_string(const QJsonObject &object, const QString &field_name,
                     const QString &a_default = "") -> QString;

void cannot_open_error(const QString &filename);

void json_instrument_error(const QString &instrument);

[[nodiscard]] auto has_instrument(
    const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &maybe_instrument) -> bool;

void error_instrument(const QString &instrument, bool interactive);

void error_row(size_t row);

void error_column(int column);

void error_empty();

void extract_instruments(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &orchestra_code);

void fill_combo_box(QComboBox &combo_box,
                    std::vector<std::unique_ptr<const QString>> &text_pointers);

void set_combo_box(QComboBox &combo_box, const QString &value);

[[nodiscard]] auto verify_json_string(const QJsonValue &json_value, const QString &field_name)
    -> bool;

[[nodiscard]] auto require_json_field(const QJsonObject &json_object,
                        const QString &field_name) -> bool;

void warn_unrecognized_field(const QString &level, const QString &field);

[[nodiscard]] auto verify_json_instrument(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QJsonObject &json_object, const QString &field_name) -> bool;

[[nodiscard]] auto verify_json_array(const QJsonValue &json_value, const QString &field_name)
    -> bool;

[[nodiscard]] auto verify_json_object(const QJsonValue &json_value, const QString &field_name)
    -> bool;

[[nodiscard]] auto verify_bounded_int(const QJsonObject &json_object,
                           const QString &field_name, double minimum,
                           double maximum) -> bool;

[[nodiscard]] auto verify_bounded_double(const QJsonObject &json_object,
                           const QString &field_name, double minimum,
                           double maximum) -> bool;

void error_level(TreeLevel level);