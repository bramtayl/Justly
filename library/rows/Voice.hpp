#pragma once

#include <QtWidgets/QMessageBox>

#include "cell_types/Program.hpp"
#include "cell_types/Rational.hpp"
#include "rows/Row.hpp"

class QWidget;

struct Voice {
  QString name;
  QString program;
  Rational velocity_ratio;
};

template <typename SubVoice>  // type properties
concept VoiceInterface = std::derived_from<SubVoice, Voice> &&
                         requires(const SubVoice& voice, xmlNode& node) {
                           {
                             SubVoice::get_pitched()
                           } -> std::same_as<const char*>;
                           // see row_to_xml
                           voice.to_xml(node);
                           { SubVoice::is_pitched() } -> std::same_as<bool>;
                           { SubVoice::get_name_column() } -> std::same_as<int>;
                         };

[[nodiscard]] inline auto get_voice_program(const QList<Program>& programs,
                                            const Voice& voice)
    -> const Program& {
  const auto result_index = get_named_index(programs, voice.program);
  Q_ASSERT(result_index != programs.cend());
  return *result_index;
}

// every note in a song names an existing voice, so only call this for notes
// already in the song, not ones freshly parsed from XML
template <VoiceInterface SubVoice>
[[nodiscard]] auto get_voice(const QList<SubVoice>& voices, const QString& name)
    -> const SubVoice& {
  const auto result_index = get_named_index(voices, name);
  Q_ASSERT(result_index != voices.cend());
  return *result_index;
}

// the voice's position, e.g. for its MIDI track or piano roll color
template <VoiceInterface SubVoice>
[[nodiscard]] auto get_voice_number(const QList<SubVoice>& voices,
                                    const QString& name) -> int {
  const auto result_index = get_named_index(voices, name);
  Q_ASSERT(result_index != voices.cend());
  return static_cast<int>(result_index - voices.cbegin());
}

// false if no voice has that name, e.g. a note pasted from another song, or
// copied before its voice was renamed or removed
template <VoiceInterface SubVoice>
[[nodiscard]] auto has_voice(const QList<SubVoice>& voices, const QString& name)
    -> bool {
  return get_named_index(voices, name) != voices.cend();
}

template <VoiceInterface SubVoice>
[[nodiscard]] auto check_voice_name(QWidget& parent,
                                    const QList<SubVoice>& voices,
                                    const int column_number,
                                    const QVariant& new_value) -> bool {
  if (column_number != SubVoice::get_name_column()) {
    return true;
  }
  const auto new_string = variant_to<QString>(new_value);
  if (new_string.isEmpty()) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Voice name is empty!");
    QMessageBox::warning(&parent, QObject::tr("Voice name error"), message);
    return false;
  }
  if (get_named_index(voices, new_string) != voices.cend()) {
    QString message;
    QTextStream stream(&message);
    stream << "Voice \"" << new_string << QObject::tr("\" already exists!");
    QMessageBox::warning(&parent, QObject::tr("Voice name error"), message);
    return false;
  }
  return true;
}
