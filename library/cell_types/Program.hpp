#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <algorithm>
#include <iterator>

struct FluidSynth;

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

  Program(const char * name_input,  short bank_number_input,
           short preset_number_input,
           double release_milliseconds_input);
};

Q_DECLARE_METATYPE(const Program *);

[[nodiscard]] auto
is_pitched_bank_number( short bank_number) -> bool;

[[nodiscard]] auto get_some_programs( bool is_pitched)
    -> const QList<Program> &;

[[nodiscard]] auto get_some_program_names( bool is_pitched)
    -> const QList<QString> &;
