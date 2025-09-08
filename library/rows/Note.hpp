#pragma once

#include "cell_types/Rational.hpp"
#include "rows/Row.hpp"
#include "sound/Player.hpp"

static const auto BREATH_ID = 2;
static const auto MAX_RELEASE_TIME = 6000;
static const auto MAX_VELOCITY = 127;

struct Note : Row {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  [[nodiscard]] virtual auto
  get_closest_midi(QWidget &parent, Player &player, int channel_number,
                   int chord_number,
                   int note_number) const -> std::optional<short> = 0;

  [[nodiscard]] virtual auto
  get_program_pointer(QWidget &parent, const PlayState &play_state,
                      int chord_number,
                      int note_number) const -> const Program * = 0;
};

template <typename SubNote> // type properties
concept NoteInterface = std::derived_from<SubNote, Note> &&
  requires()
{
  { SubNote::get_description() } -> std::same_as<const char *>;
};

template <NoteInterface SubNote>
static void add_note_location(QTextStream &stream, const int chord_number,
                              const int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1
         << QObject::tr(SubNote::get_description()) << note_number + 1;
}
