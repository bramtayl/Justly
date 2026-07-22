#pragma once

#include <QtCore/QStandardPaths>
#include <QtWidgets/QMenu>
#include <algorithm>
#include <utility>

#include "iterators/MostRecentIterator.hpp"
#include "iterators/TimeIterator.hpp"
#include "musicxml/MeasureRepeatInfo.hpp"
#include "musicxml/PartInfo.hpp"
#include "other/Song.hpp"
#include "sound/Player.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SwitchColumn.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/XMLValidator.hpp"

static const auto DEFAULT_REPEAT_TIMES = 2;
static const auto FIFTH_HALFSTEPS = 7;
static const auto START_END_MILLISECONDS = 500;
static const auto VOICE_PREVIEW_MILLISECONDS = 1000;

[[nodiscard]] static auto get_property(xmlNode &node, const char *name) {
  return xml_string_to_string(xmlGetProp(&node, c_string_to_xml_string(name)));
}

[[nodiscard]] static auto get_optional_property(xmlNode &node, const char *name)
    -> std::string {
  auto *const value_pointer = xmlGetProp(&node, c_string_to_xml_string(name));
  if (value_pointer == nullptr) {
    return {};
  }
  return xml_string_to_string(value_pointer);
}

struct SongWidget : public QWidget {
  Song song;
  Player player = Player(*this);
  QUndoStack undo_stack = QUndoStack(nullptr);
  QString current_file;
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  SwitchColumn &switch_column = *(new SwitchColumn(undo_stack, song));
  ControlsColumn &controls_column = *(new ControlsColumn(
      song, player.synth, undo_stack, switch_column.switch_table));
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit SongWidget() {
    row_layout.addWidget(&controls_column, 0, Qt::AlignTop);
    row_layout.addWidget(&switch_column, 0, Qt::AlignTop);
  }

  ~SongWidget() override { undo_stack.disconnect(); }

  NO_MOVE_COPY(SongWidget)
};

[[nodiscard]] static inline auto get_next_row(const SongWidget &song_widget) {
  return get_only_range(song_widget.switch_column.switch_table).bottom() + 1;
}

static void initialize_play(SongWidget &song_widget) {
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  initialize_playstate(
      song, player.play_state,
      fluid_sequencer_get_tick(player.sequencer.internal_pointer));

  auto &channel_schedules = player.channel_schedules;
  for (auto index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

[[nodiscard]] static auto
get_free_channel_number(const QList<double> &channel_schedules) {
  return static_cast<int>(
      std::distance(std::begin(channel_schedules),
                    std::ranges::min_element(channel_schedules)));
}

static void play_note(Player &player, const int channel_number,
                      const Program &program, const short midi_number,
                      const short velocity, const double current_time,
                      const double end_time) {
  auto &sequencer = player.sequencer;
  auto &event = player.event;
  const auto soundfont_id = player.soundfont_id;

  fluid_event_program_select(event.internal_pointer, channel_number,
                             soundfont_id, program.bank_number,
                             program.preset_number);
  send_event_at(sequencer, event, current_time);

  fluid_synth_cc(player.synth.internal_pointer, channel_number, BREATH_ID,
                 velocity);
  fluid_event_noteon(event.internal_pointer, channel_number, midi_number,
                     velocity);
  send_event_at(sequencer, event, current_time);

  fluid_event_noteoff(event.internal_pointer, channel_number, midi_number);
  send_event_at(sequencer, event, end_time);

  player.channel_schedules[channel_number] = end_time + MAX_RELEASE_TIME;
}

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
    const auto channel_number =
        get_free_channel_number(player.channel_schedules);

    const auto &voice = voices.at(voice_number);

    const auto &program = get_voice_program(programs, voices, voice_number);

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
    const auto channel_number =
        get_free_channel_number(player.channel_schedules);
    const auto &sub_note = sub_notes.at(note_number);

    const auto &program =
        sub_note.get_program(pitched_voices, unpitched_voices);

    const auto midi_number =
        sub_note.get_closest_midi(parent, player, unpitched_voices,
                                  channel_number, chord_number, note_number);

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

static void update_final_time(Player &player, const double new_final_time) {
  player.final_time = std::max(new_final_time, player.final_time);
}

static void play_chords(SongWidget &song_widget, const int first_chord_number,
                        const int number_of_chords, const int wait_frames = 0) {
  auto &player = song_widget.player;
  auto &play_state = player.play_state;
  const auto &song = song_widget.song;

  const auto &pitched_voices = song.pitched_voices;
  const auto &unpitched_voices = song.unpitched_voices;

  const auto start_time = player.play_state.current_time + wait_frames;
  play_state.current_time = start_time;
  update_final_time(player, start_time);
  const auto &chords = song.chords;
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);

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

[[nodiscard]] static inline auto can_discard_changes(SongWidget &song_widget) {
  return song_widget.undo_stack.isClean() ||
         QMessageBox::question(&song_widget, SongWidget::tr("Unsaved changes"),
                               SongWidget::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

static inline auto get_gain(const SongWidget &song_widget) -> double {
  return fluid_synth_get_gain(song_widget.player.synth.internal_pointer);
}

static inline void export_to_file(SongWidget &song_widget,
                                  const QString &output_file) {
  Q_ASSERT(output_file.isValidUtf16());
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  auto &settings = player.settings;
  auto &event = player.event;
  auto &sequencer = player.sequencer;
  auto &driver = player.driver;

  stop_playing(sequencer, event);

  if (driver.internal_pointer != nullptr) {
    delete_fluid_audio_driver(driver.internal_pointer);
  }
  driver.internal_pointer = nullptr;

  set_fluid_string(settings, "audio.file.name",
                   output_file.toStdString().c_str());

  set_fluid_int(settings, "synth.lock-memory", 0);

  auto finished = false;
  const auto finished_timer_id = fluid_sequencer_register_client(
      player.sequencer.internal_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        get_reference(static_cast<bool *>(data_pointer)) = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(song_widget);
  play_chords(song_widget, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  set_destination(event, finished_timer_id);
  fluid_event_timer(event.internal_pointer, nullptr);
  send_event_at(sequencer, event, player.final_time + START_END_MILLISECONDS);

  auto &renderer =
      get_reference(new_fluid_file_renderer(player.synth.internal_pointer));
  while (!finished) {
    check_fluid_ok(fluid_file_renderer_process_block(&renderer));
  }
  delete_fluid_file_renderer(&renderer);

  set_destination(event, player.sequencer.sequencer_id);
  set_fluid_int(settings, "synth.lock-memory", 1);
  player.driver =
      make_audio_driver(player.parent, player.settings, player.synth);
}

static void set_xml_double(xmlNode &node, const char *const field_name,
                           double value) {
  set_xml_string(node, field_name, std::to_string(value));
}

static inline void save_as_file(SongWidget &song_widget,
                                const QString &filename) {
  Q_ASSERT(filename.isValidUtf16());
  const auto &song = song_widget.song;

  XMLDocument document;
  auto &song_node = make_root(document, "song");

  set_xml_double(song_node, "gain", get_gain(song_widget));
  set_xml_double(song_node, "starting_key", song.starting_key);
  set_xml_double(song_node, "starting_tempo", song.starting_tempo);
  set_xml_double(song_node, "starting_velocity", song.starting_velocity);

  maybe_set_xml_rows(song_node, "chords", song.chords);
  maybe_set_xml_rows(song_node, "pitched_voices", song.pitched_voices);
  maybe_set_xml_rows(song_node, "unpitched_voices", song.unpitched_voices);

  xmlSaveFile(filename.toStdString().c_str(), document.internal_pointer);

  song_widget.current_file = filename;

  song_widget.undo_stack.setClean();
}

[[nodiscard]] static auto check_xml_document(QWidget &parent,
                                             XMLDocument &document) {
  if (document.internal_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("XML error"),
                         QObject::tr("Invalid XML file"));
    return false;
  }
  return true;
}

[[nodiscard]] static auto maybe_read_xml_file(const QString &filename) {
  return XMLDocument(xmlReadFile(filename.toStdString().c_str(), nullptr, 0));
}

template <RowInterface SubRow>
static void clear_rows(RowsModel<SubRow> &rows_model) {
  const auto number_of_rows = rows_model.rowCount(QModelIndex());
  if (number_of_rows > 0) {
    rows_model.remove_rows(0, number_of_rows);
  }
}

[[nodiscard]] static auto xml_to_double(const xmlNode &element) {
  return std::stod(get_content(element));
}

[[nodiscard]] static inline auto
validate_against_schema(XMLValidator &validator, XMLDocument &document) {
  return xmlSchemaValidateDoc(validator.context.internal_pointer,
                              document.internal_pointer);
}

template <VoiceInterface SubVoice>
[[nodiscard]] static auto
check_duplicate_or_empty_voice_names(QWidget &parent,
                                     const QList<SubVoice> &voices) -> bool {
  for (const auto &voice : voices) {
    if (voice.name.isEmpty()) {
      QMessageBox::warning(&parent, QObject::tr("Voice name error"),
                           QObject::tr("Voice name is empty!"));
      return false;
    }
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

[[nodiscard]] static auto check_voice_names(QWidget &parent, const Song &song)
    -> bool {
  return check_duplicate_or_empty_voice_names(parent, song.pitched_voices) &&
         check_duplicate_or_empty_voice_names(parent, song.unpitched_voices);
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

[[nodiscard]] static auto check_chord_voices(QWidget &parent, const Song &song)
    -> bool {
  const auto number_of_pitched_voices =
      static_cast<int>(song.pitched_voices.size());
  const auto number_of_unpitched_voices =
      static_cast<int>(song.unpitched_voices.size());
  for (auto chord_number = 0; chord_number < song.chords.size();
       chord_number = chord_number + 1) {
    const auto &chord = song.chords.at(chord_number);
    if (!check_note_voices(parent, chord.pitched_notes,
                           number_of_pitched_voices, chord_number) ||
        !check_note_voices(parent, chord.unpitched_notes,
                           number_of_unpitched_voices, chord_number)) {
      return false;
    }
  }
  return true;
}

static inline void open_file(SongWidget &song_widget, const QString &filename) {

  Q_ASSERT(filename.isValidUtf16());
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &chords_model = switch_table.chords_model;
  auto &unpitched_voices_model = switch_table.unpitched_voices_model;
  auto &pitched_voices_model = switch_table.pitched_voices_model;

  auto document = maybe_read_xml_file(filename);
  if (!check_xml_document(song_widget, document)) {
    return;
  }

  static XMLValidator song_validator("song.xsd");
  if (validate_against_schema(song_validator, document) != 0) {
    QMessageBox::warning(&song_widget, QObject::tr("Validation Error"),
                         QObject::tr("Invalid song file"));
    return;
  }

  clear_rows(chords_model);
  clear_rows(pitched_voices_model);
  clear_rows(unpitched_voices_model);

  auto &song_node = get_root(document);

  auto *field_pointer = xmlFirstElementChild(&song_node);
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "gain") {
      spin_boxes.gain_editor.setValue(xml_to_double(field_node));
    } else if (name == "starting_key") {
      spin_boxes.starting_key_editor.setValue(xml_to_double(field_node));
    } else if (name == "starting_velocity") {
      spin_boxes.starting_velocity_editor.setValue(xml_to_double(field_node));
    } else if (name == "starting_tempo") {
      spin_boxes.starting_tempo_editor.setValue(xml_to_double(field_node));
    } else if (name == "chords") {
      chords_model.insert_xml_rows(0, field_node);
    } else if (name == "pitched_voices") {
      pitched_voices_model.insert_xml_rows(0, field_node);
    } else if (name == "unpitched_voices") {
      unpitched_voices_model.insert_xml_rows(0, field_node);
    } else {
      Q_ASSERT(false);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }

  if (!check_voice_names(song_widget, song_widget.song) ||
      !check_chord_voices(song_widget, song_widget.song)) {
    clear_rows(chords_model);
    clear_rows(pitched_voices_model);
    clear_rows(unpitched_voices_model);
    return;
  }

  song_widget.current_file = filename;

  clear_and_clean(undo_stack);
}

[[nodiscard]] static auto node_is(const xmlNode &node, const char *name) {
  return get_xml_name(node) == name;
}

[[nodiscard]] static auto get_xml_child(xmlNode &node, const char *name)
    -> auto & {
  xmlNode *result_pointer = nullptr;
  auto *child_pointer = xmlFirstElementChild(&node);
  while (child_pointer != nullptr) {
    if (node_is(get_reference(child_pointer), name)) {
      result_pointer = child_pointer;
      break;
    }
    child_pointer = xmlNextElementSibling(child_pointer);
  }
  return get_reference(result_pointer);
}

[[nodiscard]] static auto get_duration(xmlNode &measure_element) {
  return xml_to_int(get_xml_child(measure_element, "duration"));
}

[[nodiscard]] static auto get_interval(const int midi_interval) {
  const auto [octave, degree] = get_octave_degree(midi_interval);
  static const QList<Rational> scale = {
      Rational(1, 1), Rational(16, 15), Rational(9, 8),   Rational(6, 5),
      Rational(5, 4), Rational(4, 3),   Rational(45, 32), Rational(3, 2),
      Rational(8, 5), Rational(5, 3),   Rational(9, 5),   Rational(15, 8)};
  return Interval(scale[degree], octave);
}

[[nodiscard]] static auto get_max_duration(const QList<MusicXMLNote> &notes) {
  if (notes.empty()) {
    return 0;
  }
  return std::ranges::max_element(notes,
                                  [](const MusicXMLNote &first_note,
                                     const MusicXMLNote &second_note) {
                                    return first_note.duration <
                                           second_note.duration;
                                  })
      ->duration;
}

static void add_chord(ChordsModel &chords_model,
                      const MusicXMLChord &parse_chord,
                      const int measure_number, const int key,
                      const int last_midi_key, const int song_divisions,
                      const int time_delta) {
  Chord new_chord;
  new_chord.beats = Rational(time_delta, song_divisions);
  new_chord.interval = get_interval(key - last_midi_key);
  new_chord.words = QString::number(measure_number);
  auto &unpitched_notes = new_chord.unpitched_notes;
  for (const auto &parse_unpitched_note : parse_chord.unpitched_notes) {
    UnpitchedNote new_note;
    new_note.beats = Rational(parse_unpitched_note.duration, song_divisions);
    new_note.words = parse_unpitched_note.words;
    new_note.voice_number = parse_unpitched_note.voice_number;
    unpitched_notes.push_back(std::move(new_note));
  }
  auto &pitched_notes = new_chord.pitched_notes;
  for (const auto &parse_pitched_note : parse_chord.pitched_notes) {
    PitchedNote new_note;
    new_note.beats = Rational(parse_pitched_note.duration, song_divisions);
    new_note.words = parse_pitched_note.words;
    new_note.interval = get_interval(parse_pitched_note.midi_number - key);
    new_note.voice_number = parse_pitched_note.voice_number;
    pitched_notes.push_back(std::move(new_note));
  }
  chords_model.insert_row(chords_model.rowCount(QModelIndex()),
                          std::move(new_chord));
}

static void add_note(MusicXMLChord &chord, MusicXMLNote note, bool is_pitched) {
  (is_pitched ? chord.pitched_notes : chord.unpitched_notes)
      .push_back(std::move(note));
}

static void add_note_and_maybe_chord(QMap<int, MusicXMLChord> &chords_dict,
                                     MusicXMLNote note, bool is_pitched) {
  const auto start_time = note.start_time;
  if (chords_dict.contains(start_time)) {
    add_note(chords_dict[start_time], std::move(note), is_pitched);
  } else {
    MusicXMLChord new_chord;
    add_note(new_chord, std::move(note), is_pitched);
    chords_dict[start_time] = std::move(new_chord);
  }
}

[[nodiscard]] static auto get_most_recent(MostRecentIterator &iterator,
                                          const int time) -> int {
  auto &iterator_state = iterator.state;
  auto &iterator_value = iterator.value;
  const auto &iterator_end = iterator.end;
  while (iterator_state != iterator_end && iterator_state.key() <= time) {
    iterator_value = iterator_state.value();
    ++iterator_state;
  }
  return iterator_value;
}

static void reset(TimeIterator &iterator) {
  iterator.state = iterator.dict.begin();
  iterator.last_change_time = 0;
  iterator.next_change_divisions_time = 0;
  iterator.time_per_division = 1;
}

[[nodiscard]] static auto parse_ending_numbers(xmlNode &ending_node) {
  QList<int> numbers;
  const auto numbers_text =
      QString::fromStdString(get_property(ending_node, "number"));
  for (const auto &token : numbers_text.split(',', Qt::SkipEmptyParts)) {
    bool is_number = false;
    const auto number = token.trimmed().toInt(&is_number);
    if (is_number) {
      numbers.push_back(number);
    }
  }
  return numbers;
}

// records forward/backward repeats and first/second-ending brackets onto
// the current measure, so the raw per-part timeline can be unrolled below
static void parse_barline(xmlNode &barline_node,
                          QList<int> &active_ending_numbers,
                          MeasureRepeatInfo &measure_info) {
  auto *child_pointer = xmlFirstElementChild(&barline_node);
  while (child_pointer != nullptr) {
    auto &child = get_reference(child_pointer);
    if (node_is(child, "repeat")) {
      const auto direction = get_property(child, "direction");
      if (direction == "forward") {
        measure_info.has_forward_repeat = true;
      } else if (direction == "backward") {
        measure_info.has_backward_repeat = true;
        const auto times_text = get_optional_property(child, "times");
        measure_info.repeat_times =
            times_text.empty() ? DEFAULT_REPEAT_TIMES : std::stoi(times_text);
      }
    } else if (node_is(child, "ending")) {
      if (get_property(child, "type") == "start") {
        for (const auto number : parse_ending_numbers(child)) {
          if (!active_ending_numbers.contains(number)) {
            active_ending_numbers.push_back(number);
          }
          if (!measure_info.ending_numbers.contains(number)) {
            measure_info.ending_numbers.push_back(number);
          }
        }
      } else { // "stop" or "discontinue"
        active_ending_numbers.clear();
      }
    }
    child_pointer = xmlNextElementSibling(child_pointer);
  }
}

// turns a linear list of measures (each optionally tagged with a
// forward/backward repeat and/or first-/second-ending numbers) into an
// ordered list of (start_time, end_time) spans describing the actual
// playback order, unrolling repeated sections and picking the ending that
// belongs to each pass
[[nodiscard]] static auto
compute_measure_expansion(const QList<MeasureRepeatInfo> &measure_infos) {
  QList<std::pair<int, int>> expansion;
  const auto number_of_measures = static_cast<int>(measure_infos.size());
  auto repeat_start_index = -1;
  auto block_start_index = 0;

  const auto flush = [&](const int first_index, const int last_index) {
    for (auto index = first_index; index <= last_index; index = index + 1) {
      const auto &measure_info = measure_infos.at(index);
      expansion.push_back({measure_info.start_time, measure_info.end_time});
    }
  };

  auto measure_index = 0;
  while (measure_index < number_of_measures) {
    const auto &measure_info = measure_infos.at(measure_index);
    if (measure_info.has_forward_repeat) {
      flush(block_start_index, measure_index - 1);
      repeat_start_index = measure_index;
      block_start_index = measure_index;
    }
    if (measure_info.has_backward_repeat) {
      const auto start_index =
          repeat_start_index == -1 ? 0 : repeat_start_index;
      if (repeat_start_index == -1) {
        flush(block_start_index, start_index - 1);
      }
      // a later ending (e.g. the second ending) has no repeat barline of
      // its own; it just continues on directly after the measure with the
      // backward repeat, so absorb any immediately-following ending measures
      auto block_end_index = measure_index;
      while (block_end_index + 1 < number_of_measures &&
             !measure_infos.at(block_end_index + 1).ending_numbers.isEmpty()) {
        block_end_index = block_end_index + 1;
      }
      for (auto pass_number = 1; pass_number <= measure_info.repeat_times;
           pass_number = pass_number + 1) {
        for (auto inner_index = start_index; inner_index <= block_end_index;
             inner_index = inner_index + 1) {
          const auto &inner_measure = measure_infos.at(inner_index);
          if (inner_measure.ending_numbers.isEmpty() ||
              inner_measure.ending_numbers.contains(pass_number)) {
            expansion.push_back(
                {inner_measure.start_time, inner_measure.end_time});
          }
        }
      }
      measure_index = block_end_index;
      block_start_index = block_end_index + 1;
      repeat_start_index = -1;
    }
    measure_index = measure_index + 1;
  }
  flush(block_start_index, number_of_measures - 1);
  return expansion;
}

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

[[nodiscard]] static auto
get_time_and_time_per_division(TimeIterator &iterator,
                               const int check_divisions_time) {
  auto &iterator_state = iterator.state;
  const auto song_divisions = iterator.song_divisions;
  const auto &iterator_end = iterator.end;
  while (iterator_state != iterator_end) {
    const auto next_change_divisions_time = iterator_state.key();
    if (next_change_divisions_time > check_divisions_time) {
      break;
    }
    const auto divisions_delta =
        next_change_divisions_time - iterator.next_change_divisions_time;
    iterator.next_change_divisions_time = next_change_divisions_time;
    const auto next_divisions = iterator_state.value();
    Q_ASSERT(next_divisions > 0);
    const auto time_per_division = song_divisions / next_divisions;
    iterator.time_per_division = time_per_division;
    iterator.last_change_time =
        iterator.last_change_time + time_per_division * divisions_delta;
    iterator.next_change_divisions_time = next_change_divisions_time;
    iterator_state++;
  }
  const auto time_per_division = iterator.time_per_division;
  return std::make_tuple(
      iterator.last_change_time +
          (time_per_division *
           (check_divisions_time - iterator.next_change_divisions_time)),
      time_per_division);
}

[[nodiscard]] static auto get_or_create_voice_number(
    QMap<QString, int> &voice_numbers, QList<QString> &voice_names,
    const QString &voice_key, const QString &voice_name) -> int {
  const auto found_voice_number = voice_numbers.find(voice_key);
  if (found_voice_number != voice_numbers.end()) {
    return found_voice_number.value();
  }
  const auto new_voice_number = static_cast<int>(voice_names.size());
  voice_numbers[voice_key] = new_voice_number;
  voice_names.push_back(voice_name);
  return new_voice_number;
}

[[nodiscard]] static auto deduplicate_voice_names(QList<QString> voice_names)
    -> QList<QString> {
  QSet<QString> used_names;
  for (auto &voice_name : voice_names) {
    if (voice_name.isEmpty()) {
      voice_name = QObject::tr("Unnamed instrument");
    }
    auto candidate_name = voice_name;
    for (auto suffix_number = 2; used_names.contains(candidate_name);
         suffix_number = suffix_number + 1) {
      candidate_name = voice_name + QString(" (%1)").arg(suffix_number);
    }
    voice_name = candidate_name;
    used_names.insert(candidate_name);
  }
  return voice_names;
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

static inline void import_musicxml(SongWidget &song_widget,
                                   const QString &filename) {
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &chords_model = song_widget.switch_column.switch_table.chords_model;
  auto &pitched_voices_model =
      song_widget.switch_column.switch_table.pitched_voices_model;
  auto &unpitched_voices_model =
      song_widget.switch_column.switch_table.unpitched_voices_model;

  auto document = maybe_read_xml_file(filename);
  if (!check_xml_document(song_widget, document)) {
    return;
  }

  static XMLValidator musicxml_validator("musicxml.xsd");
  if (validate_against_schema(musicxml_validator, document) != 0) {
    QMessageBox::warning(&song_widget, QObject::tr("Validation Error"),
                         QObject::tr("Invalid musicxml file"));
    return;
  }

  // Get root_pointer element
  auto &score_partwise = get_root(document);
  if (!node_is(score_partwise, "score-partwise")) {
    QMessageBox::warning(
        &song_widget, QObject::tr("Partwise error"),
        QObject::tr("Justly only supports partwise musicxml scores"));
    return; // endpoint
  }

  // Get part-list
  QMap<std::string, PartInfo> part_info_dict;

  QMap<int, MusicXMLNote> tied_notes;
  auto song_divisions = 1;

  QMap<QString, int> pitched_voice_numbers;
  QList<QString> pitched_voice_names;
  QMap<QString, int> unpitched_voice_numbers;
  QList<QString> unpitched_voice_names;

  auto *part_node_pointer = xmlFirstElementChild(&score_partwise);
  while (part_node_pointer != nullptr) {
    auto &part_node = get_reference(part_node_pointer);
    const auto part_node_name = get_xml_name(part_node);
    if (part_node_name == "part-list") {
      auto *score_part_pointer = xmlFirstElementChild(&part_node);
      while (score_part_pointer != nullptr) {
        auto &score_part = get_reference(score_part_pointer);
        if (node_is(score_part, "score-part")) {
          PartInfo part_info;
          auto &instrument_map = part_info.instrument_map;
          auto *field_pointer = xmlFirstElementChild(score_part_pointer);
          while (field_pointer != nullptr) {
            auto &field_node = get_reference(field_pointer);
            const auto child_name = get_xml_name(field_node);
            if (child_name == "part-name") {
              part_info.part_name = get_qstring_content(field_node);
            } else if (child_name == "score-instrument") {
              instrument_map[get_property(field_node, "id")] =
                  get_qstring_content(
                      get_xml_child(field_node, "instrument-name"));
            }
            field_pointer = xmlNextElementSibling(field_pointer);
          }
          part_info_dict[get_property(score_part, "id")] = std::move(part_info);
        }
        score_part_pointer = xmlNextElementSibling(score_part_pointer);
      }
    } else if (part_node_name == "part") {
      const auto part_id = get_property(part_node, "id");
      auto &part_info = part_info_dict[part_id];

      auto &part_chords_dict = part_info.part_chords_dict;
      auto &part_divisions_dict = part_info.part_divisions_dict;
      auto &part_measure_number_dict = part_info.part_measure_number_dict;
      auto &part_midi_keys_dict = part_info.part_midi_keys_dict;

      auto current_time = 0;
      auto chord_start_time = current_time;
      auto measure_number = 1;
      auto current_transpose_semitones = 0;

      QList<MeasureRepeatInfo> measure_infos;
      QList<int> active_ending_numbers;

      auto *measure_pointer = xmlFirstElementChild(&part_node);
      while (measure_pointer != nullptr) {
        auto &measure = get_reference(measure_pointer);
        part_measure_number_dict[current_time] = measure_number;
        MeasureRepeatInfo measure_info;
        measure_info.start_time = current_time;
        measure_info.ending_numbers = active_ending_numbers;
        auto *measure_element_pointer = xmlFirstElementChild(&measure);
        while (measure_element_pointer != nullptr) {
          auto &measure_element = get_reference(measure_element_pointer);
          const auto measure_element_name = get_xml_name(measure_element);
          if (measure_element_name == "attributes") {
            auto *attribute_element_pointer =
                xmlFirstElementChild(&measure_element);
            while (attribute_element_pointer != nullptr) {
              auto &attribute_element =
                  get_reference(attribute_element_pointer);
              const auto attribute_name = get_xml_name(attribute_element);
              if (attribute_name == "key") {
                const auto [octave, degree] = get_octave_degree(
                    FIFTH_HALFSTEPS *
                    xml_to_int(get_xml_child(attribute_element, "fifths")));
                part_midi_keys_dict[current_time] = MIDDLE_C_MIDI + degree;
              } else if (attribute_name == "divisions") {
                const auto new_divisions = xml_to_int(attribute_element);
                Q_ASSERT(new_divisions > 0);
                song_divisions = std::lcm(song_divisions, new_divisions);
                part_divisions_dict[current_time] = new_divisions;
              } else if (attribute_name == "transpose") {
                const auto chromatic_semitones =
                    xml_to_int(get_xml_child(attribute_element, "chromatic"));
                auto octave_change_octaves = 0;
                auto *transpose_field_pointer =
                    xmlFirstElementChild(&attribute_element);
                while (transpose_field_pointer != nullptr) {
                  auto &transpose_field =
                      get_reference(transpose_field_pointer);
                  if (node_is(transpose_field, "octave-change")) {
                    octave_change_octaves = xml_to_int(transpose_field);
                  }
                  transpose_field_pointer =
                      xmlNextElementSibling(transpose_field_pointer);
                }
                current_transpose_semitones =
                    chromatic_semitones +
                    octave_change_octaves * HALFSTEPS_PER_OCTAVE;
              }
              attribute_element_pointer =
                  xmlNextElementSibling(attribute_element_pointer);
            }
          } else if (measure_element_name == "note") {
            auto note_duration = 0;
            auto midi_number = -1;
            bool is_pitched = true;
            bool tie_start = false;
            bool tie_end = false;
            bool new_chord = true;
            bool is_rest = false;
            QString instrument_name = "";
            std::string instrument_id;

            static const QMap<std::string, int> note_to_midi = {
                {"C", 0},   {"C#", 1}, {"Db", 1}, {"D", 2},  {"D#", 3},
                {"Eb", 3},  {"E", 4},  {"F", 5},  {"F#", 6}, {"Gb", 6},
                {"G", 7},   {"G#", 8}, {"Ab", 8}, {"A", 9},  {"A#", 10},
                {"Bb", 10}, {"B", 11}};

            auto *note_field_pointer =
                xmlFirstElementChild(measure_element_pointer);
            while ((note_field_pointer != nullptr)) {
              auto &note_field = get_reference(note_field_pointer);
              const auto &name = get_xml_name(note_field);
              if (name == "pitch") {
                auto midi_degree = 0;
                auto octave_number = 0;
                auto alter = 0;

                auto *pitch_field_pointer = xmlFirstElementChild(&note_field);
                while (pitch_field_pointer != nullptr) {
                  auto &pitch_field = get_reference(pitch_field_pointer);
                  const auto &pitch_field_name = get_xml_name(pitch_field);
                  if (pitch_field_name == "step") {
                    midi_degree = note_to_midi[get_content(pitch_field)];
                  } else if (pitch_field_name == "octave") {
                    octave_number = xml_to_int(pitch_field);
                  } else if (pitch_field_name == "alter") {
                    alter = xml_to_int(pitch_field);
                  }
                  pitch_field_pointer =
                      xmlNextElementSibling(pitch_field_pointer);
                }
                midi_number = midi_degree + alter +
                              octave_number * HALFSTEPS_PER_OCTAVE + C_0_MIDI;
              } else if (name == "duration") {
                note_duration = xml_to_int(note_field);
              } else if (name == "unpitched") {
                is_pitched = false;
              } else if (name == "tie") {
                const auto tie_type = get_property(note_field, "type");
                if (tie_type == "stop") {
                  tie_end = true;
                } else if (tie_type == "start") {
                  tie_start = true;
                }
              } else if (name == "chord") {
                new_chord = false;
              } else if (name == "rest") {
                is_rest = true;
              } else if (name == "instrument") {
                instrument_id = get_property(note_field, "id");
                instrument_name = part_info.instrument_map[instrument_id];
              }
              note_field_pointer = xmlNextElementSibling(note_field_pointer);
            }

            if (is_pitched) {
              midi_number += current_transpose_semitones;
            }

            if (note_duration == 0) {
              QMessageBox::warning(
                  &song_widget, QObject::tr("Note duration error"),
                  QObject::tr("Notes without durations not supported"));
              return; // endpoint
            }
            if (new_chord) {
              chord_start_time = current_time;
              current_time += note_duration;
            }
            if (!is_rest) {
              if (tie_end) {
                const auto tied_notes_iterator = tied_notes.find(midi_number);
                Q_ASSERT(tied_notes_iterator != tied_notes.end());
                auto &previous_note = tied_notes_iterator.value();
                previous_note.duration = previous_note.duration + note_duration;
                if (!tie_start) {
                  add_note_and_maybe_chord(part_chords_dict, previous_note,
                                           is_pitched);
                  tied_notes.erase(tied_notes_iterator);
                }
              } else {
                MusicXMLNote new_note;
                new_note.duration = note_duration;
                QTextStream stream(&new_note.words);
                stream << QObject::tr("Part ") << part_info.part_name;
                if (instrument_name != "") {
                  stream << QObject::tr(" instrument ") << instrument_name;
                }
                new_note.midi_number = midi_number;
                new_note.start_time = chord_start_time;
                auto &voice_numbers = is_pitched ? pitched_voice_numbers
                                                 : unpitched_voice_numbers;
                auto &voice_names =
                    is_pitched ? pitched_voice_names : unpitched_voice_names;
                QString voice_key;
                QTextStream voice_key_stream(&voice_key);
                voice_key_stream << QString::fromStdString(part_id) << ":"
                                 << QString::fromStdString(instrument_id);
                new_note.voice_number = get_or_create_voice_number(
                    voice_numbers, voice_names, voice_key,
                    instrument_name.isEmpty() ? part_info.part_name
                                              : instrument_name);
                if (tie_start) { // also not tie end
                  tied_notes[midi_number] = std::move(new_note);
                } else { // not tie start or end
                  add_note_and_maybe_chord(part_chords_dict,
                                           std::move(new_note), is_pitched);
                }
              }
            }
          } else if (measure_element_name == "backup") {
            current_time -= get_duration(measure_element);
            chord_start_time = current_time;
          } else if (measure_element_name == "forward") {
            current_time += get_duration(measure_element);
            chord_start_time = current_time;
          } else if (measure_element_name == "barline") {
            parse_barline(measure_element, active_ending_numbers, measure_info);
          }
          measure_element_pointer =
              xmlNextElementSibling(measure_element_pointer);
        }
        measure_info.end_time = current_time;
        measure_infos.push_back(std::move(measure_info));
        measure_number++;
        measure_pointer = xmlNextElementSibling(&measure);
      }
      const auto expansion = compute_measure_expansion(measure_infos);
      part_info.part_chords_dict =
          remap_by_expansion(part_info.part_chords_dict, expansion);
      part_info.part_divisions_dict =
          remap_by_expansion(part_info.part_divisions_dict, expansion);
      part_info.part_midi_keys_dict =
          remap_by_expansion(part_info.part_midi_keys_dict, expansion);
      part_info.part_measure_number_dict =
          remap_by_expansion(part_info.part_measure_number_dict, expansion);
    }
    part_node_pointer = xmlNextElementSibling(part_node_pointer);
  }

  QMap<int, MusicXMLChord> chords_dict;
  QMap<int, int> midi_keys_dict;
  QMap<int, int> measure_number_dict;

  for (auto [part_id, part_info] : part_info_dict.asKeyValueRange()) {
    TimeIterator time_iterator(part_info.part_divisions_dict, song_divisions);
    for (auto [divisions_time, chord] :
         part_info.part_chords_dict.asKeyValueRange()) {
      auto [time, time_per_division] =
          get_time_and_time_per_division(time_iterator, divisions_time);
      auto &new_pitched_notes = chord.pitched_notes;
      auto &new_unpitched_notes = chord.unpitched_notes;
      for (auto &pitched_note : new_pitched_notes) {
        pitched_note.duration = pitched_note.duration * time_per_division;
      }
      for (auto &unpitched_note : new_unpitched_notes) {
        unpitched_note.duration = unpitched_note.duration * time_per_division;
      }
      if (chords_dict.contains(time)) {
        auto &old_chord = chords_dict[time];

        old_chord.pitched_notes.append(std::move(new_pitched_notes));
        old_chord.unpitched_notes.append(std::move(new_unpitched_notes));
      } else {
        chords_dict[time] = std::move(chord);
      }
    }

    reset(time_iterator);
    for (const auto [divisions_time, measure_number] :
         part_info.part_measure_number_dict.asKeyValueRange()) {
      auto [time, time_per_division] =
          get_time_and_time_per_division(time_iterator, divisions_time);
      measure_number_dict[time] = measure_number;
    }

    reset(time_iterator);
    for (const auto [divisions_time, midi_key] :
         part_info.part_midi_keys_dict.asKeyValueRange()) {
      auto [time, time_per_division] =
          get_time_and_time_per_division(time_iterator, divisions_time);
      midi_keys_dict[time] = midi_key;
    }
  }

  auto chord_state = chords_dict.begin();
  const auto chord_dict_end = chords_dict.end();

  if (chord_state == chord_dict_end) {
    QMessageBox::warning(&song_widget, QObject::tr("Empty MusicXML error"),
                         QObject::tr("No chords"));
    return; // endpoint
  }

  if (unpitched_voice_names.empty()) {
    // a file with no percussion/unpitched notes would otherwise leave
    // song.unpitched_voices completely empty, so manually inserting any
    // unpitched note afterward (which defaults its voice_number to 0)
    // would reference a voice that doesn't exist
    unpitched_voice_names.push_back(QObject::tr("unpitched voice 1"));
  }

  clear_rows(chords_model);
  clear_rows(pitched_voices_model);
  clear_rows(unpitched_voices_model);
  add_imported_voices(pitched_voices_model,
                      deduplicate_voice_names(pitched_voice_names));
  add_imported_voices(unpitched_voices_model,
                      deduplicate_voice_names(unpitched_voice_names));

  MostRecentIterator measure_number_iterator(measure_number_dict, 1);
  MostRecentIterator midi_key_iterator(midi_keys_dict, DEFAULT_STARTING_MIDI);

  auto time = chord_state.key();

  auto parse_chord = std::move(chord_state.value());

  auto midi_key = get_most_recent(midi_key_iterator, time);

  spin_boxes.starting_key_editor.setValue(midi_number_to_frequency(midi_key));

  auto last_midi_key = midi_key;

  ++chord_state;
  while (chord_state != chord_dict_end) {
    const auto next_time = chord_state.key();
    add_chord(chords_model, parse_chord,
              get_most_recent(measure_number_iterator, time), midi_key,
              last_midi_key, song_divisions, next_time - time);

    time = next_time;
    parse_chord = std::move(chord_state.value());

    last_midi_key = midi_key;
    midi_key = get_most_recent(midi_key_iterator, time);

    ++chord_state;
  }
  add_chord(chords_model, parse_chord,
            get_most_recent(measure_number_iterator, time), midi_key,
            last_midi_key, song_divisions,
            std::max(get_max_duration(parse_chord.pitched_notes),
                     get_max_duration(parse_chord.unpitched_notes)));

  clear_and_clean(undo_stack);
}

static inline void add_menu_action(
    QMenu &menu, QAction &action,
    const QKeySequence::StandardKey key_sequence = QKeySequence::UnknownKey,
    const bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}
