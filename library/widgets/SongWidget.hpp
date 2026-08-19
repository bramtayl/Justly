#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <libxml/parser.h>
#include <optional>
#include <string>
#include <utility>

#include "cell_types/Program.hpp"
#include "cell_types/Rational.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/Row.hpp"
#include "rows/Voice.hpp"
#include "sound/PlayState.hpp"
#include "sound/Player.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SwitchColumn.hpp"

template <RowInterface SubRow> struct RowsModel;
class QMenu;
class XMLDocument;
struct MeasureRepeatInfo;
struct PitchedVoice;
struct TimeIterator;
struct UnpitchedVoice;
struct XMLValidator;

[[nodiscard]] auto get_property(xmlNode &node, const char *name) -> std::string;

struct SongWidget : public QWidget {
  Song song;
  Player player;
  QUndoStack undo_stack;
  QString current_file;
  QString current_folder;

  // debounced autosave for crash recovery -- restarted on every undo_stack
  // change and wired up by connect_recovery_timer once save_as_file and
  // friends are defined later in this header (see comment there)
  QTimer &recovery_timer;

  SwitchColumn &switch_column;
  ControlsColumn &controls_column;
  QBoxLayout &row_layout;

  explicit SongWidget();

  ~SongWidget() override;

  NO_MOVE_COPY(SongWidget)
};

[[nodiscard]] auto get_next_row(const SongWidget &song_widget) -> int;

void initialize_play(SongWidget &song_widget);

// pitched notes always pick from the shared least-recently-free pool, since
// each one may need its own pitch bend and must wait out the previous
// occupant's release before reusing its channel. Percussion programs instead
// get a single channel permanently reserved on first use (see
// Player::percussion_channels) -- nullopt means every channel is claimed and
// the caller should warn and abort, matching channel_is_free's contract
[[nodiscard]] auto
get_channel_number(QWidget &parent, Player &player, const Program &program,
                    double current_time) -> std::optional<int>;

void play_note(Player &player,  int channel_number,
              const Program &program,  short midi_number,
               short velocity,  double current_time,
               double end_time);

template <VoiceInterface SubVoice>
[[nodiscard]] static auto
play_voices(Player &player, const QList<SubVoice> &voices,
            const int first_voice_number, const int number_of_voices) -> bool {
  static const auto VOICE_PREVIEW_MILLISECONDS = 1000;

  auto &parent = player.parent;

  const auto current_time = player.play_state.current_time;
  const auto current_velocity = player.play_state.current_velocity;

  const auto &programs = get_some_programs(SubVoice::is_pitched());

  for (auto voice_number = first_voice_number;
       voice_number < first_voice_number + number_of_voices;
       voice_number = voice_number + 1) {
    const auto &voice = voices.at(voice_number);

    const auto &program = get_voice_program(programs, voices, voice_number);

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
[[nodiscard]] static auto
play_notes(Player &player, const QList<PitchedVoice> &pitched_voices,
           const QList<UnpitchedVoice> &unpitched_voices,
           const int chord_number, const QList<SubNote> &sub_notes,
           const int first_note_number, const int number_of_notes) {
  auto &parent = player.parent;

  const auto current_time = player.play_state.current_time;
  const auto current_velocity = player.play_state.current_velocity;
  const auto current_tempo = player.play_state.current_tempo;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    const auto &sub_note = sub_notes.at(note_number);

    const auto &program =
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

    const auto &voice_velocity_ratio =
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
[[nodiscard]] static auto
play_all_notes(Player &player, const QList<PitchedVoice> &pitched_voices,
               const QList<UnpitchedVoice> &unpitched_voices,
               const int chord_number, const QList<SubNote> &sub_notes)
    -> bool {
  return play_notes(player, pitched_voices, unpitched_voices, chord_number,
                    sub_notes, 0, static_cast<int>(sub_notes.size()));
}

void update_final_time(Player &player,  double new_final_time);

void play_chords(SongWidget &song_widget,  int first_chord_number,
                  int number_of_chords,  int wait_frames = 0);

[[nodiscard]] auto can_discard_changes(SongWidget &song_widget) -> bool;

[[nodiscard]] auto get_gain(const SongWidget &song_widget) -> double;

void export_to_file(SongWidget &song_widget, const QString &output_file);

void export_midi_to_file(SongWidget &song_widget,
                         const QString &output_file);

// recovery.xml's presence means the app didn't reach a clean shutdown last
// time (see connect_recovery_timer and SongEditor::closeEvent); its content
// mirrors save_as_file's format so it can be reloaded via open_file
[[nodiscard]] auto get_recovery_file_path() -> QString;

void remove_recovery_file();

void write_recovery_file(SongWidget &song_widget);

void save_as_file(SongWidget &song_widget, const QString &filename);

// some musicxml fields (e.g. fifths, octave-change, repeat times) are
// unbounded xs:integer with no schema-enforced range, so a malformed or
// hostile file can contain a magnitude that overflows int; used by
// import_musicxml to reject such a file with a warning instead of letting
// string_to_int assert
template <RowInterface SubRow>
static void clear_rows(RowsModel<SubRow> &rows_model) {
  const auto number_of_rows = rows_model.rowCount(QModelIndex());
  if (number_of_rows > 0) {
    rows_model.remove_rows(0, number_of_rows);
  }
}

[[nodiscard]] auto
validate_against_schema(XMLValidator &validator, XMLDocument &document) -> int;

template <VoiceInterface SubVoice>
[[nodiscard]] static auto
check_duplicate_or_empty_voice_names(QWidget &parent,
                                     const QList<SubVoice> &voices) -> bool {
  if (std::ranges::any_of(voices, [](const SubVoice &voice) -> auto {
        return voice.name.isEmpty();
      })) {
    QMessageBox::warning(&parent, QObject::tr("Voice name error"),
                         QObject::tr("Voice name is empty!"));
    return false;
  }
  QSet<QString> seen_names;
  for (const auto &voice : voices) {
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

template <NoteInterface SubNote>
[[nodiscard]] static auto
check_note_voices(QWidget &parent, const QList<SubNote> &notes,
                  const int number_of_voices, const int chord_number) -> bool {
  for (auto note_number = 0; note_number < notes.size();
       note_number = note_number + 1) {
    const auto voice_number = notes.at(note_number).voice_number;
    if (voice_number < 0 || voice_number >= number_of_voices) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Voice ") << voice_number;
      add_note_location<SubNote>(stream, chord_number, note_number);
      stream << QObject::tr(" has no corresponding voice");
      QMessageBox::warning(&parent, QObject::tr("Voice number error"), message);
      return false;
    }
  }
  return true;
}

[[nodiscard]] auto open_file(SongWidget &song_widget,
                             const QString &filename) -> bool;

// call after SongEditor is constructed and shown: recovery.xml only exists
// if the previous session didn't reach a clean shutdown (see
// connect_recovery_timer and SongEditor::closeEvent). Returns whether a
// recovery was actually loaded, so callers know whether to refresh
[[nodiscard]] auto
maybe_restore_recovery(SongWidget &song_widget) -> bool;

void connect_recovery_timer(SongWidget &song_widget);

void reset(TimeIterator &iterator);

// turns a linear list of measures (each optionally tagged with a
// forward/backward repeat and/or first-/second-ending numbers) into an
// ordered list of (start_time, end_time) spans describing the actual
// playback order, unrolling repeated sections and picking the ending that
// belongs to each pass
[[nodiscard]] auto
compute_measure_expansion(const QList<MeasureRepeatInfo> &measure_infos)
    -> QList<std::pair<int, int>>;

// replays a raw per-part dict (keyed by the original, un-repeated division
// time) onto the unrolled timeline described by an expansion computed by
// compute_measure_expansion
template <typename Value>
[[nodiscard]] static auto
remap_by_expansion(const QMap<int, Value> &raw_dict,
                   const QList<std::pair<int, int>> &expansion) {
  QMap<int, Value> expanded_dict;
  auto new_cursor = 0;
  for (const auto &[raw_start, raw_end] : expansion) {
    for (auto iterator = raw_dict.lowerBound(raw_start);
         iterator != raw_dict.end() && iterator.key() < raw_end; ++iterator) {
      expanded_dict[new_cursor + (iterator.key() - raw_start)] =
          iterator.value();
    }
    new_cursor = new_cursor + (raw_end - raw_start);
  }
  return expanded_dict;
}

template <VoiceInterface SubVoice>
static void add_imported_voices(RowsModel<SubVoice> &voices_model,
                                const QList<QString> &voice_names) {
  const auto &programs = get_some_programs(SubVoice::is_pitched());
  for (const auto &voice_name : voice_names) {
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

[[nodiscard]] auto import_musicxml(SongWidget &song_widget,
                                   const QString &filename) -> bool;

void add_menu_action(
    QMenu &menu, QAction &action,
     QKeySequence::StandardKey key_sequence = QKeySequence::UnknownKey,
     bool enabled = true);
