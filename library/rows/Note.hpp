#pragma once

#include "cell_types/Rational.hpp"
#include "rows/Row.hpp"

struct PitchedVoice;
struct Player;
struct Program;
struct UnpitchedVoice;

static const auto MAX_VELOCITY = 127;

struct Note : Row {
  QString voice_name;
  Rational beats;
  Rational velocity_ratio;
  QString words;

  // nullopt means the note is unplayable as-is (e.g. a pitched note whose
  // frequency is out of MIDI range) and the caller should abort rather than
  // play a bogus note
  [[nodiscard]] virtual auto get_closest_midi(
      QWidget& parent, Player& player,
      const QList<UnpitchedVoice>& unpitched_voices, int channel_number,
      int chord_number, int note_number) const -> std::optional<short> = 0;

  [[nodiscard]] virtual auto get_program(
      const QList<PitchedVoice>& pitched_voices,
      const QList<UnpitchedVoice>& unpitched_voices) const
      -> const Program& = 0;

  [[nodiscard]] virtual auto get_voice_velocity_ratio(
      const QList<PitchedVoice>& pitched_voices,
      const QList<UnpitchedVoice>& unpitched_voices) const
      -> const Rational& = 0;

  // scales the chord's velocity by this note's ratio and its voice's ratio
  [[nodiscard]] auto get_velocity(
      const double current_velocity, const QList<PitchedVoice>& pitched_voices,
      const QList<UnpitchedVoice>& unpitched_voices) const -> double {
    return current_velocity * rational_to_double(velocity_ratio) *
           rational_to_double(
               get_voice_velocity_ratio(pitched_voices, unpitched_voices));
  }
};

template <typename SubNote>  // type properties
concept NoteInterface = std::derived_from<SubNote, Note> && requires() {
  { SubNote::get_pitched() } -> std::same_as<const char*>;
};

template <NoteInterface SubNote>
static void add_note_location(QTextStream& stream, const int chord_number,
                              const int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1 << QObject::tr(", ")
         << QObject::tr(SubNote::get_pitched()) << QObject::tr(" note ")
         << note_number + 1;
}
