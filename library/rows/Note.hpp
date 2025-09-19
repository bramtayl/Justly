#pragma once

#include "cell_types/Rational.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "sound/Player.hpp"

static const auto BREATH_ID = 2;
static const auto MAX_RELEASE_TIME = 6000;
static const auto MAX_VELOCITY = 127;

struct Note : Row {
  QString voice;
  Rational beats;
  Rational velocity_ratio;
  QString words;

  [[nodiscard]] virtual auto
  get_closest_midi(QWidget &parent, Player &player,
                   const QList<UnpitchedVoice> &unpitched_voices,
                   int channel_number, int chord_number,
                   int note_number) const -> short = 0;

  [[nodiscard]] virtual auto
  get_program_pointer(QWidget &parent,
                      const QList<PitchedVoice> &pitched_voices,
                      const QList<UnpitchedVoice> &unpitched_voices,
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

template <NoteInterface SubNote, VoiceInterface SubVoice>
[[nodiscard]] auto
get_note_program_pointer(QWidget &parent,
                         const QList<SubVoice> &voices, const SubNote &note,
                         const int chord_number,
                         const int note_number) -> const Program * {
  const auto *voice_pointer = get_voice_pointer(
      voices, note.voice);
  if (voice_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("No ");
    stream << QObject::tr(SubNote::get_description());
    add_note_location<SubNote>(stream, chord_number, note_number);
    QMessageBox::warning(&parent, QObject::tr("Voice error"), message);
  }
  return get_reference(voice_pointer).program_pointer;
}
