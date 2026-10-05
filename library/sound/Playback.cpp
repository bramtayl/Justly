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

auto get_channel_number(QWidget& parent, Player& player, const Program& program,
                        const double current_time) -> std::optional<int> {
  if (!is_pitched_bank_number(program.bank_number)) {
    auto& percussion_channels = player.percussion_channels;
    const auto existing = percussion_channels.constFind(&program);
    if (existing != percussion_channels.constEnd()) {
      return existing.value();
    }
  }

  const auto channel_number = pick_channel_index(player.channel_schedules);
  if (!channel_is_free(parent, player.channel_schedules, channel_number,
                       current_time)) {
    return std::nullopt;
  }

  if (!is_pitched_bank_number(program.bank_number)) {
    // claimed forever: play_note skips the usual release-time reschedule for
    // percussion channels, so this channel drops out of the pool for good
    player.channel_schedules[channel_number] =
        std::numeric_limits<double>::max();
    player.percussion_channels[&program] = channel_number;
  }
  return channel_number;
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

  const auto& pitched_voices = song.pitched_voices;
  const auto& unpitched_voices = song.unpitched_voices;

  const auto start_time = player.play_state.current_time + wait_frames;
  play_state.current_time = start_time;
  update_final_time(player, start_time);
  const auto& chords = song.chords;
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto& chord = chords.at(chord_number);

    modulate(play_state, chord);
    const auto pitched_result =
        play_all_notes(player, pitched_voices, unpitched_voices, chord_number,
                       chord.pitched_notes);
    if (!pitched_result) {
      return;
    }
    const auto unpitched_result =
        play_all_notes(player, pitched_voices, unpitched_voices, chord_number,
                       chord.unpitched_notes);
    if (!unpitched_result) {
      return;
    }
    move_time(play_state, chord);
    update_final_time(player, play_state.current_time);
  }
}

void export_to_file(Player& player, const Song& song,
                    const QString& output_file) {
  static const auto START_END_MILLISECONDS = 500;
  Q_ASSERT(output_file.isValidUtf16());

  auto& settings = player.settings;
  auto& event = player.event;
  auto& sequencer = player.sequencer;
  auto& driver = player.driver;

  stop_playing(sequencer, event);

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
