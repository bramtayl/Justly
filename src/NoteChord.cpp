#include "NoteChord.h"

#include <QtCore/qglobal.h>        // for qCritical
// #include <QtCore/qtcoreexports.h>  // for qUtf8Printable
// #include <bits/std_abs.h>          // for abs
#include <qbytearray.h>            // for QByteArray

#include <cstdlib>  // for abs
#include <limits>   // for numeric_limits

auto NoteChord::error_column(int column) -> void {
  qCritical("No column %d", column);
}

void NoteChord::assert_field_count(const QJsonObject &object,
                                   int expected_size) {
  auto actual_size = object.size();
  // don't include children
  if (object.contains("children")) {
    actual_size = actual_size - 1;
  }
  if (actual_size != expected_size) {
    qCritical("Actual size %i doesn't match expected size %i!",
              static_cast<int>(actual_size), expected_size);
  }
}

auto NoteChord::get_field(const QJsonObject &object, const QString &name)
    -> QJsonValue {
  if (!object.contains(name)) {
    qCritical("No field %s!", qUtf8Printable(name));
  }
  return object[name];
}

auto NoteChord::get_string(const QJsonObject &object, const QString &name)
    -> QString {
  auto json_field = NoteChord::get_field(object, name);
  if (!json_field.isString()) {
    qCritical("Non-string field %s!", qUtf8Printable(name));
  }
  return json_field.toString();
}

auto NoteChord::get_double(const QJsonObject &object, const QString &name)
    -> double {
  auto json_field = NoteChord::get_field(object, name);
  if (!json_field.isDouble()) {
    qCritical("Non-numeric field %s!", qUtf8Printable(name));
  }
  return json_field.toDouble();
}

auto NoteChord::get_positive_double(const QJsonObject &object,
                                    const QString &name) -> double {
  auto double_field = NoteChord::get_double(object, name);
  if (!(double_field > 0)) {
    qCritical("Non-positive %s: %f!", qUtf8Printable(name), double_field);
  }
  return double_field;
}

auto NoteChord::get_int(const QJsonObject &object, const QString &name) -> int {
  auto double_field = NoteChord::get_double(object, name);
  auto int_field = static_cast<int>(double_field);
  if (!(abs(double_field - int_field) <=
        std::numeric_limits<double>::epsilon())) {
    qCritical("Non-integer field %s: %f!", qUtf8Printable(name), double_field);
  }
  return static_cast<int>(double_field);
}

auto NoteChord::get_positive_int(const QJsonObject &object, const QString &name)
    -> int {
  auto int_field = NoteChord::get_int(object, name);
  if (!(int_field > 0)) {
    qCritical("Non-positive %s: %i!", qUtf8Printable(name), int_field);
  }
  return int_field;
}

auto NoteChord::get_non_negative_int(const QJsonObject &object,
                                     const QString &name) -> int {
  auto int_field = NoteChord::get_int(object, name);
  if (!(int_field >= 0)) {
    qCritical("Negative %s: %i!", qUtf8Printable(name), int_field);
  }
  return int_field;
}
