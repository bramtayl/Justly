#pragma once

#include <qjsonobject.h>         // for QJsonObject
#include <qjsonvalue.h>          // for QJsonValue
#include <qregularexpression.h>  // for QRegularExpressionMatch
#include <qstring.h>             // for QString

#include <cstddef>  // for size_t
#include <vector>   // for vector

class Instrument;
class QComboBox;  // lines 10-10

const auto SMALLER_MARGIN = 5;

void json_parse_error(const QString &error_text);

[[nodiscard]] auto get_json_double(const QJsonObject &object,
                                   const QString &field_name, double a_default)
    -> double;
[[nodiscard]] auto get_json_int(const QJsonObject &object,
                                const QString &field_name, int a_default)
    -> int;
[[nodiscard]] auto get_json_string(const QJsonObject &object,
                                   const QString &field_name,
                                   const QString &a_default = "") -> QString;

void cannot_open_error(const QString &filename);

void json_instrument_error(const QString &instrument);

[[nodiscard]] auto has_instrument(const std::vector<Instrument> &instruments,
                                  const QString &maybe_instrument) -> bool;

void error_row(size_t row);

void error_column(int column);

void error_empty(const QString &action);

void set_combo_box(QComboBox &combo_box, const QString &value);

[[nodiscard]] auto verify_json_string(const QJsonValue &json_value,
                                      const QString &field_name) -> bool;

[[nodiscard]] auto require_json_field(const QJsonObject &json_object,
                                      const QString &field_name) -> bool;

void warn_unrecognized_field(const QString &level, const QString &field);

[[nodiscard]] auto verify_json_instrument(
    const std::vector<Instrument> &instruments, const QJsonObject &json_object,
    const QString &field_name) -> bool;

[[nodiscard]] auto verify_json_array(const QJsonValue &json_value,
                                     const QString &field_name) -> bool;

[[nodiscard]] auto verify_json_object(const QJsonValue &json_value,
                                      const QString &field_name) -> bool;

[[nodiscard]] auto verify_bounded_int(const QJsonObject &json_object,
                                      const QString &field_name, double minimum,
                                      double maximum) -> bool;

[[nodiscard]] auto verify_bounded_double(const QJsonObject &json_object,
                                         const QString &field_name,
                                         double minimum, double maximum)
    -> bool;

auto get_capture_int(const QRegularExpressionMatch &match,
                     const QString &field_name, int default_value) -> int;

auto verify_regex_int(const QRegularExpressionMatch &match,
                      const QString &field_name, int minimum, int maximum)
    -> bool;

auto generate_orchestra_code(const QString &sound_font_file,
                             const std::vector<Instrument> &instruments)
    -> QString;
