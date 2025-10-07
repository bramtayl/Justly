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
