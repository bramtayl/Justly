#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QChar>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QIterator>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QTypeInfo>
#include <QtCore/Qt>
#include <QtCore/QtAssert>
#include <QtCore/QtCompare>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <fluidsynth.h>
#include <fluidsynth/audio.h>
#include <fluidsynth/event.h>
#include <fluidsynth/misc.h>
#include <fluidsynth/seq.h>
#include <fluidsynth/synth.h>
#include <fluidsynth/types.h>
#include <iterator>
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <limits>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>

#include "cell_types/Interval.hpp"
#include "cell_types/Program.hpp"
#include "cell_types/Rational.hpp"
#include "iterators/MostRecentIterator.hpp"
#include "iterators/TimeIterator.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedVoicesModel.hpp"
#include "models/UnpitchedVoicesModel.hpp"
#include "musicxml/MeasureRepeatInfo.hpp"
#include "musicxml/MusicXMLChord.hpp"
#include "musicxml/MusicXMLNote.hpp"
#include "musicxml/PartInfo.hpp"
#include "other/MidiTrackEvent.hpp"
#include "other/PianoRollNoteEvent.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Row.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedNote.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "rows/Voice.hpp"
#include "sound/FluidDriver.hpp"
#include "sound/FluidEvent.hpp"
#include "sound/FluidSequencer.hpp"
#include "sound/FluidSynth.hpp"
#include "sound/PlayState.hpp"
#include "sound/Player.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/XMLValidationContext.hpp"
#include "xml/XMLValidator.hpp"
#include "xml/ZipArchive.hpp"

template <RowInterface SubRow> struct RowsModel;

static const auto DEFAULT_REPEAT_TIMES = 2;
static const auto FIFTH_HALFSTEPS = 7;
static const auto START_END_MILLISECONDS = 500;
static const auto VOICE_PREVIEW_MILLISECONDS = 1000;
static const auto RECOVERY_DEBOUNCE_MILLISECONDS = 5000;

[[nodiscard]] auto get_property(xmlNode &node, const char *name) -> std::string;

struct SongWidget : public QWidget {
  Song song;
  Player player = Player(*this);
  QUndoStack undo_stack = QUndoStack(nullptr);
  QString current_file;
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  // debounced autosave for crash recovery -- restarted on every undo_stack
  // change and wired up by connect_recovery_timer once save_as_file and
  // friends are defined later in this header (see comment there)
  QTimer &recovery_timer = *(new QTimer(this));

  SwitchColumn &switch_column = *(new SwitchColumn(undo_stack, song));
  ControlsColumn &controls_column = *(new ControlsColumn(
      song, player.synth, undo_stack, switch_column.switch_table));
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit SongWidget();

  ~SongWidget() override;

  NO_MOVE_COPY(SongWidget)
};

[[nodiscard]] auto get_next_row(const SongWidget &song_widget) -> int;

void initialize_play(SongWidget &song_widget);

// picks whichever channel has been free the longest -- not necessarily
// actually free yet, see channel_is_free below
[[nodiscard]] auto
pick_channel_index(const QList<double> &channel_end_times) -> int;

// false means every channel is still sounding a still-releasing note at
// start_time -- there are more notes sounding at once than available MIDI
// channels, so the caller should warn and abort rather than steal a channel
// out from under a note that hasn't finished yet
[[nodiscard]] auto
channel_is_free(QWidget &parent, const QList<double> &channel_end_times,
                const int channel_index, const double start_time) -> bool;

// pitched notes always pick from the shared least-recently-free pool, since
// each one may need its own pitch bend and must wait out the previous
// occupant's release before reusing its channel. Percussion programs instead
// get a single channel permanently reserved on first use (see
// Player::percussion_channels) -- nullopt means every channel is claimed and
// the caller should warn and abort, matching channel_is_free's contract
[[nodiscard]] auto
get_channel_number(QWidget &parent, Player &player, const Program &program,
                   const double current_time) -> std::optional<int>;

void play_note(Player &player, const int channel_number,
              const Program &program, const short midi_number,
              const short velocity, const double current_time,
              const double end_time);

template <VoiceInterface SubVoice>
[[nodiscard]] static auto
play_voices(Player &player, const QList<SubVoice> &voices,
            const int first_voice_number, const int number_of_voices) -> bool {
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

void update_final_time(Player &player, const double new_final_time);

void play_chords(SongWidget &song_widget, const int first_chord_number,
                 const int number_of_chords, const int wait_frames = 0);

[[nodiscard]] auto can_discard_changes(SongWidget &song_widget) -> bool;

[[nodiscard]] auto get_gain(const SongWidget &song_widget) -> double;

void export_to_file(SongWidget &song_widget, const QString &output_file);

// 1 tick == 1 millisecond at this fixed tempo (500000 microseconds per
// quarter / 500 ticks per quarter == 1000 microseconds per tick), so the
// piano roll's already-computed absolute millisecond timestamps can be used
// directly as tick values; the declared tempo itself is arbitrary and
// doesn't reflect the song's actual tempo, which (like in live playback and
// WAV export) is already baked into those timestamps via each chord's
// tempo_ratio
static const auto MIDI_TICKS_PER_QUARTER = 500;
static const auto MIDI_MICROSECONDS_PER_QUARTER = 500000U;
static const auto MIDI_PERCUSSION_CHANNEL = 9;
static const auto MIDI_FORMAT_MULTI_TRACK = 1;
// same-tick ordering: a note-off must land before any note-on (so a
// still-sounding note doesn't get truncated), a bank select must land before
// the program change it's meant to modify, and a program change/pitch bend
// must land before the note-on it's meant to apply to
static const auto MIDI_EXPORT_NOTE_OFF_TIE_BREAK = 0;
static const auto MIDI_EXPORT_BANK_SELECT_TIE_BREAK = 1;
static const auto MIDI_EXPORT_PROGRAM_CHANGE_TIE_BREAK = 2;
static const auto MIDI_EXPORT_PITCH_BEND_TIE_BREAK = 3;
static const auto MIDI_EXPORT_BREATH_TIE_BREAK = 4;
static const auto MIDI_EXPORT_NOTE_ON_TIE_BREAK = 5;
// the standard MIDI file format has a hard 16-channel limit (a 4-bit
// channel nibble), unlike FluidSynth's own NUMBER_OF_MIDI_CHANNELS (64),
// which is an internal extension used only for live playback/WAV rendering
static const auto NUMBER_OF_STANDARD_MIDI_CHANNELS = 16;

[[nodiscard]] auto get_pitched_midi_channels() -> const QList<int> &;

void emit_note_events(QList<MidiTrackEvent> &track,
                      unsigned int channel_number,
                      unsigned int midi_number,
                      unsigned int velocity, double start_tick,
                      double end_tick);

void export_midi_to_file(SongWidget &song_widget,
                         const QString &output_file);

void set_xml_double(xmlNode &node, const char *const field_name,
                    double value);

void populate_song_document(SongWidget &song_widget,
                            XMLDocument &document);

// recovery.xml's presence means the app didn't reach a clean shutdown last
// time (see connect_recovery_timer and SongEditor::closeEvent); its content
// mirrors save_as_file's format so it can be reloaded via open_file
[[nodiscard]] auto get_recovery_file_path() -> QString;

void remove_recovery_file();

void write_recovery_file(SongWidget &song_widget);

void save_as_file(SongWidget &song_widget, const QString &filename);

[[nodiscard]] auto check_xml_document(QWidget &parent,
                                      XMLDocument &document) -> bool;

[[nodiscard]] auto maybe_read_xml_file(const QString &filename) -> XMLDocument;

// some musicxml fields (e.g. fifths, octave-change, repeat times) are
// unbounded xs:integer with no schema-enforced range, so a malformed or
// hostile file can contain a magnitude that overflows int; used by
// import_musicxml to reject such a file with a warning instead of letting
// string_to_int assert
[[nodiscard]] auto get_int_or_warn(QWidget &parent,
                                   const std::string &content,
                                   const QString &title,
                                   const QString &message)
    -> std::optional<int>;

[[nodiscard]] auto get_int_or_warn(QWidget &parent,
                                   const xmlNode &element,
                                   const QString &title,
                                   const QString &message)
    -> std::optional<int>;

template <RowInterface SubRow>
static void clear_rows(RowsModel<SubRow> &rows_model) {
  const auto number_of_rows = rows_model.rowCount(QModelIndex());
  if (number_of_rows > 0) {
    rows_model.remove_rows(0, number_of_rows);
  }
}

// loading a file replaces song.chords wholesale, which would leave
// pitched_notes_model/unpitched_notes_model pointing at destroyed Chord
// members if the switch table was drilled into a chord's notes (mirrors the
// reset that replace_table performs when the user navigates back to chords
// manually)
void reset_switch_table_to_chords(SwitchColumn &switch_column);

[[nodiscard]] auto xml_to_double(const xmlNode &element) -> double;

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

[[nodiscard]] auto node_is(const xmlNode &node, const char *name) -> bool;

[[nodiscard]] auto maybe_get_xml_child(xmlNode &node, const char *name)
    -> xmlNode *;

[[nodiscard]] auto get_xml_child(xmlNode &node, const char *name) -> xmlNode &;

[[nodiscard]] auto get_duration(QWidget &parent,
                                xmlNode &measure_element)
    -> std::optional<int>;

[[nodiscard]] auto get_interval(const int midi_interval) -> Interval;

[[nodiscard]] auto get_max_duration(const QList<MusicXMLNote> &notes) -> int;

void add_chord(ChordsModel &chords_model,
               const MusicXMLChord &parse_chord,
               const int measure_number, const int key,
               const int last_midi_key, const int song_divisions,
               const int time_delta);

void add_note(MusicXMLChord &chord, MusicXMLNote note, bool is_pitched);

void add_note_and_maybe_chord(QMap<int, MusicXMLChord> &chords_dict,
                              MusicXMLNote note, bool is_pitched);

[[nodiscard]] auto get_most_recent(MostRecentIterator &iterator,
                                   const int time) -> int;

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

[[nodiscard]] auto
get_time_and_time_per_division(TimeIterator &iterator,
                               const int check_divisions_time)
    -> std::tuple<int, int>;

[[nodiscard]] auto deduplicate_voice_names(QList<QString> voice_names)
    -> QList<QString>;

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

// a .mxl file is a zip archive; META-INF/container.xml names the entry that
// actually holds the MusicXML score (MusicXML spec, "Compressed MusicXML
// Files"). Returns an empty QByteArray on any failure, leaving the
// resulting document null so the caller's existing "Invalid XML file"
// check reports it -- avoids popping up two message boxes for one failure
[[nodiscard]] auto
maybe_read_compressed_musicxml_bytes(const QString &filename) -> QByteArray;

[[nodiscard]] auto
maybe_read_musicxml_document(const QString &filename) -> XMLDocument;

[[nodiscard]] auto import_musicxml(SongWidget &song_widget,
                                   const QString &filename) -> bool;

void add_menu_action(
    QMenu &menu, QAction &action,
    const QKeySequence::StandardKey key_sequence = QKeySequence::UnknownKey,
    const bool enabled = true);
