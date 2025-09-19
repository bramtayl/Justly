#pragma once

#include "cell_types/Program.hpp"
#include "rows/Row.hpp"

struct Voice : Row {
  QString name;
  const Program *program_pointer;
};

template <typename SubVoice> // type properties
concept VoiceInterface = std::derived_from<SubVoice, Voice> &&
  requires()
{
  { SubVoice::get_description() } -> std::same_as<const char *>;
};

template <VoiceInterface SubVoice>
auto get_voice_pointer(const QList<SubVoice> &voices, const QString &name_1) -> const SubVoice * {
  auto result_index =
      std::find_if(voices.cbegin(), voices.cend(),
                   [&name_1](const SubVoice &voice) {
                     const auto &voice_name = voice.name;
                     return voice_name == name_1;
                   });
  if (result_index != voices.cend()) {
    return nullptr;
  };
  return &(*result_index);
}