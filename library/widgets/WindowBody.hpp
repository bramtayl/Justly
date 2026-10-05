#pragma once

#include <QtCore/QModelIndex>
#include <QtGui/QUndoStack>

#include "other/Song.hpp"
#include "rows/Note.hpp"
#include "sound/Player.hpp"

template <RowInterface SubRow>
struct RowsModel;
class XMLDocument;
struct XMLValidator;
struct ControlsColumn;
struct SwitchColumn;

struct WindowBody : public QWidget {
  Song song;
  Player player;
  QUndoStack undo_stack;
  QString current_file;
  QString current_folder;

  // debounced autosave for crash recovery -- restarted on every undo_stack
  // change and wired up by connect_recovery_timer once save_as_file and
  // friends are defined later in this header (see comment there)
  QTimer& recovery_timer;

  SwitchColumn& switch_column;
  ControlsColumn& controls_column;

  explicit WindowBody();

  ~WindowBody() override;

  NO_MOVE_COPY(WindowBody)
};

[[nodiscard]] auto get_next_row(const WindowBody& window_body) -> int;

void initialize_play(WindowBody& window_body);

// pitched notes always pick from the shared least-recently-free pool, since
// each one may need its own pitch bend and must wait out the previous
// occupant's release before reusing its channel. Percussion programs instead
// get a single channel permanently reserved on first use (see
// Player::percussion_channels) -- nullopt means every channel is claimed and
// the caller should warn and abort, matching channel_is_free's contract
[[nodiscard]] auto get_channel_number(QWidget& parent, Player& player,
                                      const Program& program,
                                      double current_time)
    -> std::optional<int>;

void play_note(Player& player, int channel_number, const Program& program,
               short midi_number, short velocity, double current_time,
               double end_time);

template <VoiceInterface SubVoice>
[[nodiscard]] static auto play_voices(Player& player,
                                      const QList<SubVoice>& voices,
                                      const int first_voice_number,
                                      const int number_of_voices) -> bool {
  static const auto VOICE_PREVIEW_MILLISECONDS = 1000;

  auto& parent = player.parent;

  const auto current_time = player.play_state.current_time;
  const auto current_velocity = player.play_state.current_velocity;

  const auto& programs = get_some_programs(SubVoice::is_pitched());

  for (auto voice_number = first_voice_number;
       voice_number < first_voice_number + number_of_voices;
       voice_number = voice_number + 1) {
    const auto& voice = voices.at(voice_number);

    const auto& program = get_voice_program(programs, voice);

    const auto maybe_channel_number =
        get_channel_number(parent, player, program, current_time);
    if (!maybe_channel_number.has_value()) {
      return false;
    }
    const auto channel_number = *maybe_channel_number;

    const auto midi_number = voice.get_preview_midi_number();

    const auto velocity = static_cast<short>(std::round(
        current_velocity * rational_to_double(voice.velocity_ratio)));
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY << QObject::tr(" for ")
             << QObject::tr(SubVoice::get_pitched()) << QObject::tr(" voice \"")
             << voice.name << QObject::tr("\"");
      QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
      return false;
    }

    play_note(player, channel_number, program, midi_number, velocity,
              current_time, current_time + VOICE_PREVIEW_MILLISECONDS);
  }
  return true;
}

template <NoteInterface SubNote>
[[nodiscard]] static auto play_notes(
    Player& player, const QList<PitchedVoice>& pitched_voices,
    const QList<UnpitchedVoice>& unpitched_voices, const int chord_number,
    const QList<SubNote>& sub_notes, const int first_note_number,
    const int number_of_notes) {
  auto& parent = player.parent;

  const auto current_time = player.play_state.current_time;
  const auto current_velocity = player.play_state.current_velocity;
  const auto current_tempo = player.play_state.current_tempo;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    const auto& sub_note = sub_notes.at(note_number);

    const auto& program =
        sub_note.get_program(pitched_voices, unpitched_voices);

    const auto maybe_channel_number =
        get_channel_number(parent, player, program, current_time);
    if (!maybe_channel_number.has_value()) {
      return false;
    }
    const auto channel_number = *maybe_channel_number;

    const auto maybe_midi_number =
        sub_note.get_closest_midi(parent, player, unpitched_voices,
                                  channel_number, chord_number, note_number);
    if (!maybe_midi_number.has_value()) {
      return false;
    }
    const auto midi_number = *maybe_midi_number;

    const auto& voice_velocity_ratio =
        sub_note.get_voice_velocity_ratio(pitched_voices, unpitched_voices);
    const auto velocity = static_cast<short>(std::round(
        current_velocity * rational_to_double(sub_note.velocity_ratio) *
        rational_to_double(voice_velocity_ratio)));
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location<SubNote>(stream, chord_number, note_number);
      QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
      return false;
    }

    const auto end_time =
        current_time + get_duration_in_milliseconds(
                           current_tempo, rational_to_double(sub_note.beats));

    play_note(player, channel_number, program, midi_number, velocity,
              current_time, end_time);
  }
  return true;
}

template <NoteInterface SubNote>
[[nodiscard]] static auto play_all_notes(
    Player& player, const QList<PitchedVoice>& pitched_voices,
    const QList<UnpitchedVoice>& unpitched_voices, const int chord_number,
    const QList<SubNote>& sub_notes) -> bool {
  return play_notes(player, pitched_voices, unpitched_voices, chord_number,
                    sub_notes, 0, static_cast<int>(sub_notes.size()));
}

void update_final_time(Player& player, double new_final_time);

void play_chords(WindowBody& window_body, int first_chord_number,
                 int number_of_chords, int wait_frames = 0);

[[nodiscard]] auto can_discard_changes(WindowBody& window_body) -> bool;

[[nodiscard]] auto get_gain(const WindowBody& window_body) -> double;

void export_to_file(WindowBody& window_body, const QString& output_file);

// recovery.xml's presence means the app didn't reach a clean shutdown last
// time (see connect_recovery_timer and MainWindow::closeEvent); its content
// mirrors save_as_file's format so it can be reloaded via open_file
[[nodiscard]] auto get_recovery_file_path() -> QString;

void remove_recovery_file();

void write_recovery_file(WindowBody& window_body);

void save_as_file(WindowBody& window_body, const QString& filename);

// some musicxml fields (e.g. fifths, octave-change, repeat times) are
// unbounded xs:integer with no schema-enforced range, so a malformed or
// hostile file can contain a magnitude that overflows int; used by
// import_musicxml to reject such a file with a warning instead of letting
// string_to_int assert
template <RowInterface SubRow>
static void clear_rows(RowsModel<SubRow>& rows_model) {
  const auto number_of_rows = rows_model.rowCount(QModelIndex());
  if (number_of_rows > 0) {
    rows_model.remove_rows(0, number_of_rows);
  }
}

[[nodiscard]] auto validate_against_schema(XMLValidator& validator,
                                           XMLDocument& document) -> int;

template <VoiceInterface SubVoice>
[[nodiscard]] static auto check_duplicate_or_empty_voice_names(
    QWidget& parent, const QList<SubVoice>& voices) -> bool {
  if (std::ranges::any_of(voices, [](const SubVoice& voice) -> auto {
        return voice.name.isEmpty();
      })) {
    QMessageBox::warning(&parent, QObject::tr("Voice name error"),
                         QObject::tr("Voice name is empty!"));
    return false;
  }
  QSet<QString> seen_names;
  for (const auto& voice : voices) {
    if (seen_names.contains(voice.name)) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Duplicate voice name \"") << voice.name
             << QObject::tr("\"!");
      QMessageBox::warning(&parent, QObject::tr("Voice name error"), message);
      return false;
    }
    seen_names.insert(voice.name);
  }
  return true;
}

template <NoteInterface SubNote, VoiceInterface SubVoice>
[[nodiscard]] static auto check_note_voices(QWidget& parent,
                                            const QList<SubNote>& notes,
                                            const QList<SubVoice>& voices,
                                            const int chord_number) -> bool {
  for (auto note_number = 0; note_number < notes.size();
       note_number = note_number + 1) {
    if (!has_voice(voices, notes.at(note_number).voice_name)) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Voice");
      add_note_location<SubNote>(stream, chord_number, note_number);
      stream << QObject::tr(" has no corresponding voice");
      QMessageBox::warning(&parent, QObject::tr("Voice name error"), message);
      return false;
    }
  }
  return true;
}

[[nodiscard]] auto open_file(WindowBody& window_body, const QString& filename)
    -> bool;

// call after MainWindow is constructed and shown: recovery.xml only exists
// if the previous session didn't reach a clean shutdown (see
// connect_recovery_timer and MainWindow::closeEvent). Returns whether a
// recovery was actually loaded, so callers know whether to refresh
[[nodiscard]] auto maybe_restore_recovery(WindowBody& window_body) -> bool;

void connect_recovery_timer(WindowBody& window_body);

template <VoiceInterface SubVoice>
static void add_imported_voices(RowsModel<SubVoice>& voices_model,
                                const QList<QString>& voice_names) {
  const auto& programs = get_some_programs(SubVoice::is_pitched());
  for (const auto& voice_name : voice_names) {
    SubVoice new_voice;
    new_voice.name = voice_name;
    const auto matching_program = get_named_index(programs, voice_name);
    if (matching_program != programs.cend()) {
      new_voice.program = matching_program->name;
    }
    voices_model.insert_row(voices_model.rowCount(QModelIndex()),
                            std::move(new_voice));
  }
}

[[nodiscard]] auto import_musicxml(WindowBody& window_body,
                                   const QString& filename) -> bool;

void add_menu_action(
    QMenu& menu, QAction& action,
    QKeySequence::StandardKey key_sequence = QKeySequence::UnknownKey,
    bool enabled = true);
