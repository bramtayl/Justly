#pragma once

#include "sound/Player.hpp"
#include "rows/Row.hpp"

static const auto BREATH_ID = 2;
static const auto MAX_RELEASE_TIME = 6000;
static const auto MAX_VELOCITY = 127;

struct Note : Row {
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

static inline void send_event_at(fluid_sequencer_t &sequencer,
                                 fluid_event_t &event, const double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(fluid_sequencer_send_at(
      &sequencer, &event, static_cast<unsigned int>(std::round(time)), 1));
}

template <NoteInterface SubNote>
[[nodiscard]] static auto play_notes(Player &player, const int chord_number,
                                     const QList<SubNote> &sub_notes,
                                     const int first_note_number,
                                     const int number_of_notes) {
  auto &play_state = player.play_state;
  auto &parent = player.parent;
  auto &sequencer = player.sequencer;
  auto &event = player.event;
  auto &channel_schedules = player.channel_schedules;
  const auto soundfont_id = player.soundfont_id;

  const auto current_time = play_state.current_time;
  const auto current_velocity = play_state.current_velocity;
  const auto current_tempo = play_state.current_tempo;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    const auto channel_number = static_cast<int>(
        std::distance(std::begin(channel_schedules),
                      std::min_element(std::begin(channel_schedules),
                                       std::end(channel_schedules))));
    const auto &sub_note = sub_notes.at(note_number);

    const auto *program_pointer = sub_note.get_program_pointer(
        parent, play_state, chord_number, note_number);
    if (program_pointer == nullptr) {
      return false;
    }
    const auto &program = get_reference(program_pointer);

    fluid_event_program_select(&event, channel_number, soundfont_id,
                               program.bank_number, program.preset_number);
    send_event_at(sequencer, event, current_time);

    const auto maybe_midi_number = sub_note.get_closest_midi(
        parent, player, channel_number, chord_number, note_number);
    if (!maybe_midi_number.has_value()) {
      return false;
    }
    auto midi_number = maybe_midi_number.value();

    auto velocity = static_cast<short>(std::round(
        current_velocity * rational_to_double(sub_note.velocity_ratio)));
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location<SubNote>(stream, chord_number, note_number);
      QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
      return false;
    }
    fluid_synth_cc(&player.synth, channel_number, BREATH_ID, velocity);
    fluid_event_noteon(&event, channel_number, midi_number, velocity);
    send_event_at(sequencer, event, current_time);

    const auto end_time =
        current_time + get_milliseconds(current_tempo, sub_note.beats);

    fluid_event_noteoff(&event, channel_number, midi_number);
    send_event_at(sequencer, event, end_time);

    channel_schedules[channel_number] = end_time + MAX_RELEASE_TIME;
  }
  return true;
}
