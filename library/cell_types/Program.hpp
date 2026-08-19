#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QtAssert>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <algorithm>
#include <array>
#include <cmath>
#include <fluidsynth.h>
#include <fluidsynth/gen.h>
#include <fluidsynth/misc.h>
#include <fluidsynth/settings.h>
#include <fluidsynth/sfont.h>
#include <fluidsynth/synth.h>
#include <fluidsynth/types.h>
#include <fluidsynth/voice.h>
#include <iterator>
#include <set>
#include <string>

#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "sound/FluidSettings.hpp"
#include "sound/FluidSynth.hpp"

static const auto GENERAL_BANK_NUMBER = 0;
static const auto GENERAL_EXPRESSIVE_BANK_NUMBER = 17;
static const auto EXTRA_BANK_NUMBER = 8;
static const auto EXTRA_EXPRESSIVE_BANK_NUMBER = 18;
static const auto MAX_PITCHED_BANK_NUMBER =
    18; // banks numbers above 18 are duplicates except for detuned saw, special
        // cased below
static const auto TEMPLE_BLOCKS_BANK_NUMBER = 1;
static const auto UNPITCHED_BANK_NUMBER = 128;

[[nodiscard]] auto get_soundfont_id(FluidSynth &synth) -> int;

template <typename SubNamed>
concept NamedInterface = requires(SubNamed named) { named.name; };

template <NamedInterface Named>
auto get_named_index(const QList<Named> &nameds, const QString &name) -> auto {
  return std::find_if(
      nameds.cbegin(), nameds.cend(),
      [&name](const Named &named) -> auto { return named.name == name; });
}

template <NamedInterface Named>
[[nodiscard]] static auto get_names(const QList<Named> &nameds) {
  QList<QString> names;
  std::transform(nameds.cbegin(), nameds.cend(), std::back_inserter(names),
                 [](const Named &named) -> auto { return named.name; });
  return names;
}

struct Program {
  QString name;
  short bank_number;
  short preset_number;
  // how long a note keeps sounding after note-off before a channel can be
  // safely reused; measured from the soundfont for pitched programs (see
  // get_actual_release_milliseconds), since release time there is roughly
  // key-independent -- percussion programs map different keys to unrelated
  // drum sounds with their own envelopes, so there's no single representative
  // key to measure, and they fall back to the conservative MAX_RELEASE_TIME
  double release_milliseconds;

  Program(const char *const name_input, const short bank_number_input,
          const short preset_number_input,
          const double release_milliseconds_input);
};

Q_DECLARE_METATYPE(const Program *);

[[nodiscard]] auto
is_pitched_bank_number(const short bank_number) -> bool;

[[nodiscard]] auto get_some_programs(const bool is_pitched)
    -> const QList<Program> &;

[[nodiscard]] auto get_some_program_names(const bool is_pitched)
    -> const QList<QString> &;
