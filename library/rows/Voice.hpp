#pragma once

#include "cell_types/Program.hpp"
#include "rows/Row.hpp"

struct Voice : Row {
  QString name;
  QString program;
};

template <typename SubVoice> // type properties
concept VoiceInterface = std::derived_from<SubVoice, Voice> &&
  requires()
{
  { SubVoice::get_pitched() } -> std::same_as<const char *>;
};

template <VoiceInterface SubVoice>
[[nodiscard]] auto get_voice_program(const QList<Program> &programs,
                                     const QList<SubVoice> &voices,
                                     const QString &voice) -> const auto & {
  return get_named(programs, get_named(voices, voice).program);
}
