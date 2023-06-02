#include "Utilities.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/std_abs.h>          // for abs
#include <qbytearray.h>            // for QByteArray
#include <qcombobox.h>             // for QComboBox
#include <qcoreapplication.h>      // for QCoreApplication
#include <qjsonobject.h>           // for QJsonObject
#include <qjsonvalue.h>            // for QJsonValue
#include <qmessagebox.h>           // for QMessageBox
#include <qregularexpression.h>    // for QRegularExpressionMatchIteratorRan...
#include <qstring.h>               // for QString, operator+, operator==

#include <algorithm>  // for any_of, max
#include <cmath>      // for round
#include <cstdlib>    // for abs
#include <limits>     // for numeric_limits
#include <utility>    // for move

auto json_warning(const QString &error, const QString &field_name) {
  QMessageBox::critical(nullptr, "JSON parsing error",
                        error + " " + field_name + "!");
  QCoreApplication::exit(-1);
}

auto get_json_string(const QJsonObject &object, const QString &field_name,
                     const QString &a_default) -> QString {
  if (!object.contains(field_name)) {
    return a_default;
  }
  auto json_field = object[field_name];
  if (!json_field.isString()) {
    json_warning("Non-string", field_name);
    return a_default;
  }
  return json_field.toString();
}

auto get_json_double(const QJsonObject &object, const QString &field_name,
                     double a_default) -> double {
  if (!object.contains(field_name)) {
    return a_default;
  }
  auto json_field = object[field_name];
  if (!json_field.isDouble()) {
    json_warning("Non-double", field_name);
    return a_default;
  }
  return json_field.toDouble();
}

auto get_json_positive_double(const QJsonObject &object,
                              const QString &field_name, double a_default)
    -> double {
  auto double_field = get_json_double(object, field_name, a_default);
  if (!(double_field > 0)) {
    json_warning("Non-positive double", field_name);
    return a_default;
  }
  return double_field;
}

auto get_json_int(const QJsonObject &object, const QString &field_name,
                  int a_default) -> int {
  auto double_field = get_json_double(object, field_name, a_default * 1.0);
  auto int_field = static_cast<int>(double_field);
  if (!(abs(double_field - int_field) <=
        std::numeric_limits<double>::epsilon())) {
    json_warning("Non-integer", field_name);
    return a_default;
  }
  return static_cast<int>(std::round(double_field));
}

auto get_json_positive_int(const QJsonObject &object, const QString &field_name,
                           int a_default) -> int {
  auto int_field = get_json_int(object, field_name, a_default);
  if (!(int_field > 0)) {
    json_warning("Non-positive integer", field_name);
    return a_default;
  }
  return int_field;
}

auto get_json_non_negative_int(const QJsonObject &object,
                               const QString &field_name, int a_default)
    -> int {
  auto int_field = get_json_int(object, field_name, a_default);
  if (!(int_field >= 0)) {
    json_warning("Negative integer", field_name);
    return a_default;
  }
  return int_field;
}

void error_not_json_object() {
  QMessageBox::critical(nullptr, "JSON parsing error", "Expected JSON object!");
  QCoreApplication::exit(-1);
};

void cannot_open_error(const QString &filename) {
  qCritical("Cannot open file %s", qUtf8Printable(filename));
}

void no_instrument_error(const QString &instrument) {
  QMessageBox::critical(nullptr, "JSON parsing error",
                        QString("Cannot find instrument ") + instrument + "!");
  QCoreApplication::exit(-1);
}

auto has_instrument(
    const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &maybe_instrument) -> bool {
  if (std::any_of(instrument_pointers.cbegin(), instrument_pointers.cend(),
                  [&maybe_instrument](const auto &instrument_pointer) {
                    return *instrument_pointer == maybe_instrument;
                  })) {
    return true;
  }
  return false;
}

void error_row(size_t row) {
  qCritical("Invalid row %d", static_cast<int>(row));
};

void error_column(int column) { qCritical("No column %d", column); }

void assert_not_empty(const QModelIndexList &selected) {
  if (selected.empty()) {
    qCritical("Empty selected");
  }
}

void extract_instruments(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &orchestra_text) {
  QRegularExpression const instrument_pattern(R"(\binstr\s+\b(\w+)\b)");
  QRegularExpressionMatchIterator const instrument_matches =
      instrument_pattern.globalMatch(orchestra_text);
  for (const QRegularExpressionMatch &match : instrument_matches) {
    instrument_pointers.push_back(
        std::move(std::make_unique<QString>(match.captured(1))));
  }
}

void fill_combo_box(
    QComboBox &combo_box,
    std::vector<std::unique_ptr<const QString>> &text_pointers) {
  for (auto &text_pointer : text_pointers) {
    combo_box.addItem(*text_pointer);
  }
}

void set_combo_box(QComboBox &combo_box, const QString &value) {
  const int combo_box_index = combo_box.findText(value);
  // if it is valid, adjust the combobox
  if (combo_box_index >= 0) {
    combo_box.setCurrentIndex(combo_box_index);
  } else {
    qCritical("Cannot find ComboBox value %s", qUtf8Printable(value));
  }
}