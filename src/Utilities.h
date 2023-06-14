#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qjsonvalue.h>   // for QJsonValue
#include <qstring.h>      // for QString
#include <stddef.h>       // for size_t

#include <memory>  // for unique_ptr
#include <vector>  // for vector
class QComboBox;   // lines 10-10

auto get_json_double(const QJsonObject &object, const QString &field_name,
                     double a_default) -> double;
auto get_json_int(const QJsonObject &object, const QString &field_name,
                  int a_default) -> int;
auto get_json_string(const QJsonObject &object, const QString &field_name,
                     const QString &a_default = "") -> QString;

void cannot_open_error(const QString &filename);

auto verify_json(const QJsonObject &json_song) -> bool;

void json_instrument_error(const QString &instrument);

auto has_instrument(
    const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &maybe_instrument) -> bool;

void error_instrument(const QString &instrument, bool interactive);

void error_row(size_t row);

void error_column(int column);

void error_empty();

void extract_instruments(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &orchestra_text);

void fill_combo_box(QComboBox &combo_box,
                    std::vector<std::unique_ptr<const QString>> &text_pointers);

void set_combo_box(QComboBox &combo_box, const QString &value);

void error_root();

auto verify_json_string(const QJsonValue &json_value, const QString &field_name)
    -> bool;

auto require_json_field(const QJsonObject &json_object,
                        const QString &field_name) -> bool;

void warn_unrecognized_field(const QString &level, const QString &field);

auto verify_json_instrument(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QJsonObject &json_object, const QString &field_name) -> bool;

auto verify_json_positive(const QJsonObject &json_object,
                          const QString &field_name) -> bool;

auto verify_positive_percent(const QJsonObject &json_object,
                             const QString &field_name) -> bool;

auto verify_json_array(const QJsonValue &json_value, const QString &field_name)
    -> bool;

auto verify_json_object(const QJsonValue &json_value, const QString &field_name)
    -> bool;

auto verify_positive_int(const QJsonObject &json_object,
                         const QString &field_name) -> bool;

auto verify_whole_object(const QJsonObject json_object,
                         const QString &field_name) -> bool;

auto verify_non_negative_int(const QJsonObject &json_object,
                             const QString &field_name) -> bool;