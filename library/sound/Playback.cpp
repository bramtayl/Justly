#include "sound/Playback.hpp"

#include "rows/Chord.hpp"

void initialize_play(Player& player, const Song& song) {
  player.play_state = initialize_playstate(
      song, fluid_sequencer_get_tick(player.sequencer.internal_pointer));

  auto& channel_schedules = player.channel_schedules;
  Q_ASSERT(channel_schedules.size() == NUMBER_OF_MIDI_CHANNELS);
  std::ranges::fill(channel_schedules, 0);
  player.percussion_channels.clear();
}

namespace {

auto pick_channel_index(const QList<double>& channel_end_times) -> int {
  return static_cast<int>(
      std::distance(std::begin(channel_end_times),
                    std::ranges::min_element(channel_end_times)));
}

auto channel_is_free(QWidget& parent, const QList<double>& channel_end_times,
                     const int channel_index, const double start_time) -> bool {
  if (channel_end_times.at(channel_index) <= start_time) {
    return true;
  }
  QMessageBox::warning(
      &parent, QObject::tr("MIDI channel exhausted"),
      QObject::tr("More notes are sounding at once than there are "
                  "available MIDI channels"));
  return false;
}

}  // namespace

auto get_channel_number(Player& player, const Program& program,
                        const double current_time) -> std::optional<int> {
  const auto is_pitched = is_pitched_bank_number(program.bank_number);
  if (!is_pitched) {
    auto& percussion_channels = player.percussion_channels;
    const auto existing = percussion_channels.constFind(&program);
    if (existing != percussion_channels.constEnd()) {
      return existing.value();
    }
  }

  const auto channel_number = pick_channel_index(player.channel_schedules);
  if (!channel_is_free(player.parent, player.channel_schedules, channel_number,
                       current_time)) {
    return std::nullopt;
  }

  if (!is_pitched) {
    // claimed forever: play_note skips the usual release-time reschedule for
    // percussion channels, so this channel drops out of the pool for good
    player.channel_schedules[channel_number] =
        std::numeric_limits<double>::max();
    player.percussion_channels[&program] = channel_number;
  }
  return channel_number;
}

namespace {

void warn_frequency(QWidget& parent, const double frequency,
                    const int chord_number, const int note_number,
                    const QString& comparison, const double limit) {
  QString message;
  QTextStream stream(&message);
  stream << QObject::tr("Frequency ") << QString::number(frequency, 'g', 3);
  add_note_location<PitchedNote>(stream, chord_number, note_number);
  stream << comparison << QString::number(limit, 'g', 3);
  QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
}

}  // namespace

auto get_closest_midi(Player& player, const PitchedNote& note,
                      const int channel_number, const int chord_number,
                      const int note_number) -> std::optional<short> {
  static const auto BEND_PER_HALFSTEP = 4096;
  static const auto MAX_FREQUENCY = 12911.41;  // MIDI 127 plus half step
  static const auto QUARTER_STEP = 0.5;
  static const auto ZERO_BEND_HALFSTEPS = 2;

  auto& parent = player.parent;
  const auto& play_state = player.play_state;
  auto& event = player.event;
  const auto frequency =
      play_state.current_key * interval_to_double(note.interval);
  static const auto minimum_frequency =
      midi_number_to_frequency(0 - QUARTER_STEP);
  if (frequency < minimum_frequency) {
    warn_frequency(parent, frequency, chord_number, note_number,
                   QObject::tr(" less than minimum frequency "),
                   minimum_frequency);
    return {};
  }

  if (frequency >= MAX_FREQUENCY) {
    warn_frequency(parent, frequency, chord_number, note_number,
                   QObject::tr(" greater than or equal to maximum frequency "),
                   MAX_FREQUENCY);
    return {};
  }

  const auto midi_float = frequency_to_midi_number(frequency);
  const auto closest_midi = static_cast<short>(round(midi_float));
  fluid_event_pitch_bend(
      event.internal_pointer, channel_number,
      to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
             BEND_PER_HALFSTEP));
  send_event_at(player.sequencer, event, play_state.current_time);
  return closest_midi;
}

void play_note(Player& player, const int channel_number, const Program& program,
               const short midi_number, const short velocity,
               const double current_time, const double end_time) {
  auto& sequencer = player.sequencer;
  auto& event = player.event;
  const auto soundfont_id = player.soundfont_id;

  fluid_event_program_select(event.internal_pointer, channel_number,
                             soundfont_id, program.bank_number,
                             program.preset_number);
  send_event_at(sequencer, event, current_time);

  fluid_event_noteon(event.internal_pointer, channel_number, midi_number,
                     velocity);
  send_event_at(sequencer, event, current_time);

  fluid_event_noteoff(event.internal_pointer, channel_number, midi_number);
  send_event_at(sequencer, event, end_time);

  // a permanently-claimed percussion channel (see get_channel_number) must
  // never gain a finite schedule again, or it could look free to a pitched
  // note once that time passes, undoing the permanent claim
  if (is_pitched_bank_number(program.bank_number)) {
    player.channel_schedules[channel_number] =
        end_time + program.release_milliseconds;
  }
}

void update_final_time(Player& player, const double new_final_time) {
  player.final_time = std::max(new_final_time, player.final_time);
}

void play_chords(Player& player, const Song& song, const int first_chord_number,
                 const int number_of_chords, const int wait_frames) {
  auto& play_state = player.play_state;
  play_state.current_time = play_state.current_time + wait_frames;
  // stops at the first chord with a note that can't be played, which has
  // already warned about it
  static_cast<void>(walk_chords(
      play_state, song.chords, first_chord_number,
      first_chord_number + number_of_chords,
      [&player, &song](const int chord_number, const Chord& chord) -> bool {
        return play_all_notes(player, song, chord_number,
                              chord.pitched_notes) &&
               play_all_notes(player, song, chord_number,
                              chord.unpitched_notes);
      }));
  // time only ever moves forward, so this is the latest time played
  update_final_time(player, play_state.current_time);
}

void export_to_file(Player& player, const Song& song,
                    const QString& output_file) {
  static const auto START_END_MILLISECONDS = 500;
  Q_ASSERT(output_file.isValidUtf16());

  auto& settings = player.settings;
  auto& event = player.event;
  auto& sequencer = player.sequencer;
  auto& driver = player.driver;

  stop_playing(player);

  driver.reset();

  set_fluid_string(settings, "audio.file.name",
                   output_file.toStdString().c_str());

  set_fluid_int(settings, "synth.lock-memory", 0);

  auto finished = false;
  const auto finished_timer_id = fluid_sequencer_register_client(
      player.sequencer.internal_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t* /*event*/,
         fluid_sequencer_t* /*seq*/, void* data_pointer) -> auto {
        get_reference(static_cast<bool*>(data_pointer)) = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(player, song);
  play_chords(player, song, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  set_destination(event, finished_timer_id);
  fluid_event_timer(event.internal_pointer, nullptr);
  send_event_at(sequencer, event, player.final_time + START_END_MILLISECONDS);

  auto* const renderer_pointer =
      new_fluid_file_renderer(player.synth.internal_pointer);
  if (renderer_pointer == nullptr) {
    QMessageBox::warning(&player.parent, QObject::tr("Export error"),
                         QObject::tr("Cannot write to file"));
  } else {
    auto& renderer = get_reference(renderer_pointer);
    while (!finished) {
      if (fluid_file_renderer_process_block(&renderer) != FLUID_OK) {
        QMessageBox::warning(&player.parent, QObject::tr("Export error"),
                             QObject::tr("Error writing file"));
        break;
      }
    }
    delete_fluid_file_renderer(&renderer);
  }

  set_destination(event, player.sequencer.sequencer_id);
  set_fluid_int(settings, "synth.lock-memory", 1);
  player.driver =
      make_audio_driver(player.parent, player.settings, player.synth);
}
