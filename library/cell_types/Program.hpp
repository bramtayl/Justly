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

[[nodiscard]] static inline auto get_soundfont_id(FluidSynth &synth) {
  const auto soundfont_id =
      fluid_synth_sfload(synth.internal_pointer,
                         get_share_file("MS_Basic.sf3").c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

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
          const double release_milliseconds_input)
      // sf2 preset names have no guaranteed encoding: modern soundfonts are
      // UTF-8, but older ones sometimes use Latin-1, so fall back rather
      // than mis-decode
      : name([&]() -> QString {
          const QByteArray bytes(name_input);
          return bytes.isValidUtf8() ? QString::fromUtf8(bytes)
                                     : QString::fromLatin1(bytes);
        }()),
        bank_number(bank_number_input),
        preset_number(preset_number_input),
        release_milliseconds(release_milliseconds_input) {};
};

Q_DECLARE_METATYPE(const Program *);

[[nodiscard]] static inline auto
is_pitched_bank_number(const short bank_number) {
  return bank_number != UNPITCHED_BANK_NUMBER &&
         bank_number != TEMPLE_BLOCKS_BANK_NUMBER;
}

[[nodiscard]] static auto filter_programs(const QList<Program> &all_programs,
                                          const bool is_pitched) {
  QList<Program> result;
  std::ranges::copy_if(all_programs, std::back_inserter(result),
                       [is_pitched](const Program &program) -> auto {
                         return is_pitched_bank_number(program.bank_number) ==
                                is_pitched;
                       });
  return result;
}

// triggers the program on a scratch synth and reads back its actual volume
// envelope release time (GEN_VOLENVRELEASE, in timecents) from the resulting
// voice(s), rather than assuming the conservative MAX_RELEASE_TIME constant.
// Middle C is used as the reference key because it's the SF2 spec's reference
// key for key-scaled generators, so this reads the unscaled nominal release.
[[nodiscard]] static auto
get_actual_release_milliseconds(FluidSynth &synth,
                                fluid_preset_t *const preset_pointer)
    -> double {
  static const auto PREVIEW_KEY = 60;
  static const auto PREVIEW_VELOCITY = 100;
  static const auto MAX_MATCHING_VOICES = 16;
  // SF2 timecents are a log2 scale: seconds = 2^(timecents / 1200)
  static const auto MILLISECONDS_PER_SECOND = 1000.0;
  static const auto TIMECENTS_PER_OCTAVE = 1200.0;
  static const auto TIMECENTS_BASE = 2.0;

  // fluid_synth_stop() doesn't reclaim a voice's slot until the engine
  // actually renders past its release tail, which never happens on this
  // synth (see the polyphony comment above) -- a fixed voice ID would make
  // fluid_synth_get_voicelist() below also return every prior call's
  // still-lingering voice, contaminating this measurement with old maxima,
  // so every call gets its own never-reused ID
  static auto next_voice_id = 0U;
  const auto voice_id = next_voice_id++;

  if (fluid_synth_start(synth.internal_pointer, voice_id, preset_pointer, 0,
                        0, PREVIEW_KEY, PREVIEW_VELOCITY) != FLUID_OK) {
    return MAX_RELEASE_TIME;
  }

  std::array<fluid_voice_t *, MAX_MATCHING_VOICES> voices{};
  fluid_synth_get_voicelist(synth.internal_pointer, voices.data(),
                            MAX_MATCHING_VOICES, static_cast<int>(voice_id));

  auto release_milliseconds = 0.0;
  for (auto *const voice : voices) {
    if (voice == nullptr) {
      break;
    }
    const auto release_timecents =
        fluid_voice_gen_get(voice, GEN_VOLENVRELEASE);
    release_milliseconds = std::max(
        release_milliseconds,
        MILLISECONDS_PER_SECOND *
            std::pow(TIMECENTS_BASE, release_timecents / TIMECENTS_PER_OCTAVE));
  }

  fluid_synth_stop(synth.internal_pointer, voice_id);

  return release_milliseconds > 0 ? release_milliseconds
                                  : static_cast<double>(MAX_RELEASE_TIME);
}

[[nodiscard]] static inline auto get_some_programs(const bool is_pitched)
    -> auto & {
  static const auto all_programs = []() -> QList<Program> {
    FluidSettings settings;
    // fluid_synth_stop() releases a voice but doesn't reclaim its slot until
    // the engine actually renders past its release tail -- this synth never
    // renders audio, so without headroom well beyond the default polyphony
    // (256), later get_actual_release_milliseconds calls would silently fail
    // to start and fall back to MAX_RELEASE_TIME. This has to be set on the
    // settings before construction, not via fluid_synth_set_polyphony()
    // afterward: that call only resizes the voice-slot array, not the
    // eventhandler ringbuffer new_fluid_synth() sizes as polyphony * 64 --
    // left at the default, that ringbuffer never drains (no audio is ever
    // rendered here) and overflows partway through the hundreds of
    // start/stop calls below, logging "Ringbuffer full" for the rest
    static const auto INTROSPECTION_POLYPHONY = 8192;
    auto polyphony_was_set = fluid_settings_setint(
        settings.internal_pointer, "synth.polyphony",
        INTROSPECTION_POLYPHONY) == FLUID_OK;
    Q_ASSERT(polyphony_was_set);
    FluidSynth synth(settings);

    fluid_sfont_t *const soundfont_pointer = fluid_synth_get_sfont_by_id(
        synth.internal_pointer, get_soundfont_id(synth));
    Q_ASSERT(soundfont_pointer != nullptr);

    fluid_sfont_iteration_start(soundfont_pointer);
    auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);

    QList<Program> programs;
    std::set<int> expressive_preset_numbers;
    std::set<int> extra_expressive_preset_numbers;
    while (preset_pointer != nullptr) {
      const auto *const name = fluid_preset_get_name(preset_pointer);
      const auto bank_number =
          static_cast<short>(fluid_preset_get_banknum(preset_pointer));
      const auto preset_number =
          static_cast<short>(fluid_preset_get_num(preset_pointer));
      if (bank_number == GENERAL_EXPRESSIVE_BANK_NUMBER) {
        expressive_preset_numbers.insert(preset_number);
      }
      if (bank_number == EXTRA_EXPRESSIVE_BANK_NUMBER) {
        extra_expressive_preset_numbers.insert(preset_number);
      }
      // detuned saw expr. is the only non-duplicate instrument on a bank above
      // the max pitched bank number
      if (bank_number <= MAX_PITCHED_BANK_NUMBER ||
          bank_number == UNPITCHED_BANK_NUMBER ||
          std::string(name) == "Detuned Saw Expr.") {
        const auto release_milliseconds =
            is_pitched_bank_number(bank_number)
                ? get_actual_release_milliseconds(synth, preset_pointer)
                : static_cast<double>(MAX_RELEASE_TIME);
        programs.push_back(
            Program(name, bank_number, preset_number, release_milliseconds));
      }
      preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
    }

    const auto non_expressive_indices = std::ranges::remove_if(
        programs, [&expressive_preset_numbers,
                   &extra_expressive_preset_numbers](const auto &program) -> auto {
          const auto bank_number = program.bank_number;
          const auto preset_number = program.preset_number;
          return (bank_number == GENERAL_BANK_NUMBER &&
                  expressive_preset_numbers.find(preset_number) !=
                      expressive_preset_numbers.end()) ||
                 (bank_number == EXTRA_BANK_NUMBER &&
                  extra_expressive_preset_numbers.find(preset_number) !=
                      extra_expressive_preset_numbers.end());
        });
    programs.erase(non_expressive_indices.begin(),
                   non_expressive_indices.end());

    std::ranges::sort(
        programs, [](const Program &instrument_1, const Program &instrument_2) -> auto {
          return instrument_1.name < instrument_2.name;
        });
    return programs;
  }();
  static const auto pitched_programs = filter_programs(all_programs, true);
  static const auto unpitched_programs = filter_programs(all_programs, false);
  return is_pitched ? pitched_programs : unpitched_programs;
}

[[nodiscard]] static inline auto get_some_program_names(const bool is_pitched)
    -> auto & {
  static const auto pitched_names = get_names(get_some_programs(true));
  static const auto unpitched_names = get_names(get_some_programs(false));
  return is_pitched ? pitched_names : unpitched_names;
}
