#pragma once

#include <QtWidgets/QMessageBox>

#include "cell_types/Program.hpp"
#include "rows/Row.hpp"

struct Voice : Row {
  QString name;
  QString program;
};

template <typename SubVoice> // type properties
concept VoiceInterface = std::derived_from<SubVoice, Voice> && requires() {
  { SubVoice::get_pitched() } -> std::same_as<const char *>;
  { SubVoice::is_pitched() } -> std::same_as<bool>;
};

template <VoiceInterface SubVoice>
[[nodiscard]] auto get_voice_program(const QList<Program> &programs,
                                     const QList<SubVoice> &voices,
                                     int voice_number) -> const auto & {
  return get_named(programs, voices.at(voice_number).program);
}

template <VoiceInterface SubVoice>
[[nodiscard]] auto
check_voice_name(QWidget &parent, const QList<SubVoice> &voices,
                 const int name_column_number, const int cell_column_number,
                 const QVariant &new_value) -> bool {
  const auto new_string = variant_to<QString>(new_value);
  if (new_string.isEmpty()) {
    // TODO(brandon): test
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Voice name is empty!");
    QMessageBox::warning(&parent, QObject::tr("Voice name error"), message);
    return false;
  }
  if (name_column_number == cell_column_number) {
    const auto result_index = get_named_index(voices, new_string);
    if (result_index != voices.cend()) {
      // TODO(brandon): test
      QString message;
      QTextStream stream(&message);
      stream << "Voice \"" << new_string
             << QObject::tr("\" already exists!");
      QMessageBox::warning(&parent, QObject::tr("Voice name error"), message);
      return false;
    }
  }
  return true;
}
