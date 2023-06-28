#include "Utilities.h"

#include <QtCore/qglobal.h>       // for qCritical
#include <QtCore/qtcoreexports.h> // for qUtf8Printable
#include <qbytearray.h>           // for QByteArray
#include <qcombobox.h>            // for QComboBox
#include <qjsonobject.h>          // for QJsonObject
#include <qjsonvalue.h>           // for QJsonValue
#include <qmessagebox.h>          // for QMessageBox
#include <qregularexpression.h>   // for QRegularExpressionMatchIteratorRan...
#include <qstring.h>              // for QString, operator+, operator==

#include <algorithm> // for any_of, max
#include <limits>    // for numeric_limits
#include <utility>   // for move

#include "Instrument.h"

void json_parse_error(const QString &error_text) {
  QMessageBox::warning(nullptr, "JSON parsing error", error_text);
}

auto verify_json_string(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isString()) {
    json_parse_error(
        QString("Non-string %1: type %2!").arg(field_name).arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_double(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isDouble()) {
    json_parse_error(
        QString("Non-double %1: %2!").arg(field_name).arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_array(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isArray()) {
    json_parse_error(QString("%1 must be an array: %2!")
                         .arg(field_name)
                         .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_object(const QJsonValue &json_value, const QString &field_name)
    -> bool {
  if (!json_value.isObject()) {
    json_parse_error(QString("%1 must be an object: %2!")
                         .arg(field_name)
                         .arg(json_value.type()));
    return false;
  }
  return true;
}

auto verify_json_instrument(
    const std::vector<Instrument> &instruments,
    const QJsonObject &json_object, const QString &field_name)
    -> bool {
  const auto json_value = json_object[field_name];
  if (!(verify_json_string(json_value, field_name))) {
    return false;
  }
  const auto instrument = json_value.toString();
  if (!has_instrument(instruments, instrument)) {
    json_parse_error(
        QString("Cannot find %1 %2").arg(field_name).arg(instrument));
    return false;
  }
  return true;
}

auto verify_bounded(double value, const QString &field_name, double minimum,
                    double maximum) -> bool {
  if (value < minimum) {
    json_parse_error(QString("%1 of %2 less than minimum %3!")
                         .arg(field_name)
                         .arg(value)
                         .arg(minimum));
    return false;
  }
  if (value > maximum) {
    json_parse_error(QString("%1 of %2 greater than maximum %3!")
                         .arg(field_name)
                         .arg(value)
                         .arg(maximum));
    return false;
  }
  return true;
}

auto verify_bounded_double(const QJsonObject &json_object,
                           const QString &field_name, double minimum,
                           double maximum) -> bool {
  auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  auto value = json_value.toDouble();
  return verify_bounded(value, field_name, minimum, maximum);
}

auto verify_bounded_int(const QJsonObject &json_object,
                        const QString &field_name, double minimum,
                        double maximum) -> bool {
  auto json_value = json_object[field_name];
  if (!(verify_json_double(json_value, field_name))) {
    return false;
  }
  auto value = json_value.toDouble();
  if (!(verify_bounded(value, field_name, minimum, maximum))) {
    return false;
  }
  if ((value - static_cast<int>(value)) >
      std::numeric_limits<double>::epsilon()) {
    json_parse_error(QString("Non-whole %1 of %2!").arg(field_name).arg(value));
    return false;
  }
  return true;
}

auto get_json_string(const QJsonObject &object, const QString &field_name,
                     const QString &a_default) -> QString {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toString();
}

auto get_json_double(const QJsonObject &object, const QString &field_name,
                     double a_default) -> double {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toDouble();
}

auto get_json_int(const QJsonObject &object, const QString &field_name,
                  int a_default) -> int {
  if (!object.contains(field_name)) {
    return a_default;
  }
  return object[field_name].toInt();
}

void cannot_open_error(const QString &filename) {
  qCritical("Cannot open file %s", qUtf8Printable(filename));
}

auto has_instrument(
    const std::vector<Instrument> &instruments,
    const QString &maybe_instrument) -> bool {
  return std::any_of(instruments.cbegin(), instruments.cend(),
                     [&maybe_instrument](const auto &instrument) {
                       return instrument.display_name == maybe_instrument;
                     });
}

void error_row(size_t row) {
  qCritical("Invalid row %d", static_cast<int>(row));
};

void error_column(int column) { qCritical("No column %d", column); }

void error_empty(const QString &action) {
  qCritical("Nothing to %s!", qUtf8Printable(action));
}

void set_combo_box(QComboBox &combo_box, const QString &value) {
  const int combo_box_index = combo_box.findText(value);
  // if it is valid, adjust the combobox
  if (combo_box_index < 0) {
    qCritical("Cannot find ComboBox value %s", qUtf8Printable(value));
    return;
  }
  combo_box.setCurrentIndex(combo_box_index);
}

void error_instrument(const QString &instrument) {
  QMessageBox::warning(
      nullptr, "Instrument warning",
      QString("Cannot find instrument %1! Not changing orchestra text")
          .arg(instrument));
  // qCritical("Cannot find instrument %s", qUtf8Printable(instrument));
}

auto require_json_field(const QJsonObject &json_object,
                        const QString &field_name) -> bool {
  if (!(json_object.contains(field_name))) {
    QMessageBox::warning(nullptr, "JSON parsing error",
                         QString("No field %1!").arg(field_name));
    return false;
  }
  return true;
}

void warn_unrecognized_field(const QString &level, const QString &field) {
  json_parse_error(QString("Unrecognized %1 field %2!").arg(level).arg(field));
}

void error_level(TreeLevel level) { qCritical("Invalid level %d!", level); }

auto get_capture_int(const QRegularExpressionMatch &match,
                     const QString &field_name, int default_value) -> int {
  auto text = match.captured(field_name);
  if (text.isNull()) {
    return default_value;
  }
  return text.toInt();
}

auto verify_regex_int(const QRegularExpressionMatch &match,
                      const QString &field_name, int minimum, int maximum) -> bool {
  auto text = match.captured(field_name);
  if (!(text.isNull())) {
    auto an_int = text.toInt();
    if (an_int < minimum) {
      json_parse_error(QString("%1 %2 is less than minimum %3!")
                           .arg(field_name)
                           .arg(an_int)
                           .arg(minimum));
      return false;
    }
    if (an_int > maximum) {
      json_parse_error(QString("%1 %2 is greater than maximum %3!")
                           .arg(field_name)
                           .arg(an_int)
                           .arg(minimum));
      return false;
    }
  }
  return true;
}

auto generate_orchestra_code(const QString& sound_font_file, const std::vector<Instrument>& instruments) -> QString {
  auto orchestra_code = QString(
    "nchnls = 2\n"
    "0dbfs = 1\n"
    "\n"
    "gisound_font sfload \"%1\"\n"
    "; because 0dbfs = 1, not 32767, I guess\n"
    "gibase_amplitude = 1/32767\n"
    "; velocity is how hard you hit the key (not how loud it is)\n"
    "gimax_velocity = 100\n"
    "; short release\n"
    "girelease_duration = 0.05\n"
    "\n"
    "#define SOUND_FONT_INSTRUMENT(instrument_name'bank_number'preset_number) #\n"
    "; arguments preset number, bank_number, sound_font object, assignment_number\n"
    "gi$instrument_name sfpreset $preset_number, $bank_number, gisound_font, $preset_number\n"
    "; arguments p1 = instrument, p2 = start_time, p3 = duration, p4 = frequency, p5 = amplitude (max 1)\n"
    "instr $instrument_name\n"
    "; assume velociy is proportional to amplitude\n"
    "; arguments velocity, midi number, amplitude, frequency, preset number, ignore midi flag\n"
    "aleft_sound, aright_sound sfplay3 gimax_velocity * p5, 0, gibase_amplitude * p5, p4, gi$instrument_name, 1\n"
    "; arguments start_level, sustain_duration, mid_level, release_duration, end_level\n"
    "acutoff_envelope linsegr 1, p3, 1, girelease_duration, 0\n"
    "; cutoff instruments at end of the duration\n"
    "aleft_sound_cut = aleft_sound * acutoff_envelope\n"
    "aright_sound_cut = aright_sound * acutoff_envelope\n"
    "outs aleft_sound_cut, aright_sound_cut\n"
    "endin\n"
    "#\n"
    "\n"
  ).arg(sound_font_file);

  for (int index = 0; index < instruments.size(); index = index + 1) {
    auto& instrument = instruments[index];
    orchestra_code = orchestra_code + QString(
      ";instr %1\n"
      "$SOUND_FONT_INSTRUMENT(%1'%2'%3)\n"
      "\n"
    ).arg(instrument.code_name).arg(instrument.bank_number).arg(instrument.preset_number);
  }
  return orchestra_code;
}

auto find_instrument_code_name(const std::vector<Instrument> instruments, const QString& display_name) -> QString {
  for (const auto &instrument: instruments) {
    if (instrument.display_name == display_name) {
      return instrument.code_name;
    }
  }
  qCritical("Cannot find instrument with display name %s", qUtf8Printable(display_name));
  return {};
}