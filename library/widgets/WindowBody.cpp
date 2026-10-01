#include "widgets/WindowBody.hpp"

#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>

#include "musicxml/MusicXMLPart.hpp"
#include "other/PianoRollNoteEvent.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "xml/XMLChildren.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/XMLValidator.hpp"
#include "xml/ZipArchive.hpp"

WindowBody::WindowBody()
    : player(Player(*this)),
      undo_stack(QUndoStack(nullptr)),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      recovery_timer(*(new QTimer(this))),
      switch_column(*(new SwitchColumn(undo_stack, song))),
      controls_column(*(new ControlsColumn(song, player.synth, undo_stack,
                                           switch_column.switch_table))),
      row_layout(*(new QHBoxLayout(this))) {
  row_layout.addWidget(&controls_column, 0, Qt::AlignTop);
  row_layout.addWidget(&switch_column, 0, Qt::AlignTop);
}

WindowBody::~WindowBody() { undo_stack.disconnect(); }

auto get_next_row(const WindowBody& window_body) -> int {
  return get_only_range(window_body.switch_column.switch_table).bottom() + 1;
}

void initialize_play(WindowBody& window_body) {
  auto& player = window_body.player;
  const auto& song = window_body.song;

  initialize_playstate(
      song, player.play_state,
      fluid_sequencer_get_tick(player.sequencer.internal_pointer));

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

void play_chords(WindowBody& window_body, const int first_chord_number,
                 const int number_of_chords, const int wait_frames) {
  auto& player = window_body.player;
  auto& play_state = player.play_state;
  const auto& song = window_body.song;

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

auto can_discard_changes(WindowBody& window_body) -> bool {
  return window_body.undo_stack.isClean() ||
         QMessageBox::question(&window_body, WindowBody::tr("Unsaved changes"),
                               WindowBody::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

auto get_gain(const WindowBody& window_body) -> double {
  return fluid_synth_get_gain(window_body.player.synth.internal_pointer);
}

void export_to_file(WindowBody& window_body, const QString& output_file) {
  static const auto START_END_MILLISECONDS = 500;
  Q_ASSERT(output_file.isValidUtf16());
  auto& player = window_body.player;
  const auto& song = window_body.song;

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

  initialize_play(window_body);
  play_chords(window_body, 0, static_cast<int>(song.chords.size()),
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

namespace {

void set_xml_double(xmlNode& node, const char* const field_name, double value) {
  // std::to_string uses the current C locale, which can use a comma for the
  // decimal separator; QString::number is always locale-independent,
  // matching the xs:decimal lexical form required by song.xsd. (std::to_chars
  // would also work, but Apple's libc++ only supports the floating-point
  // overloads when targeting very recent macOS versions, and this project
  // deliberately supports macOS back to 12.0.)
  static const auto double_digits = std::numeric_limits<double>::max_digits10;
  set_xml_string(node, field_name,
                 QString::number(value, 'g', double_digits).toStdString());
}

void populate_song_document(WindowBody& window_body, XMLDocument& document) {
  const auto& song = window_body.song;

  auto& song_node = make_root(document, "song");

  set_xml_double(song_node, "gain", get_gain(window_body));
  set_xml_double(song_node, "starting_key", song.starting_key);
  set_xml_double(song_node, "starting_tempo", song.starting_tempo);
  set_xml_double(song_node, "starting_velocity", song.starting_velocity);

  const auto& pitched_voices = song.pitched_voices;
  const auto& unpitched_voices = song.unpitched_voices;
  maybe_set_xml_rows(song_node, "chords", song.chords, pitched_voices,
                     unpitched_voices);
  maybe_set_xml_rows(song_node, "pitched_voices", pitched_voices,
                     pitched_voices, unpitched_voices);
  maybe_set_xml_rows(song_node, "unpitched_voices", unpitched_voices,
                     pitched_voices, unpitched_voices);
}

}  // namespace

auto get_recovery_file_path() -> QString {
  const auto directory =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(directory);
  return directory + "/recovery.xml";
}

void remove_recovery_file() {
  QFile::remove(get_recovery_file_path());
  QSettings().remove("recovery/original_file");
}

void write_recovery_file(WindowBody& window_body) {
  XMLDocument document;
  populate_song_document(window_body, document);
  if (xmlSaveFile(get_recovery_file_path().toStdString().c_str(),
                  document.internal_pointer) < 0) {
    // leave any pre-existing recovery.xml and its original_file setting
    // alone rather than pointing them at content that was never written
    return;
  }

  // remembers where the recovered content should be saved back to, since
  // open_file (used to reload recovery.xml) always overwrites current_file
  // with whatever path it's given
  QSettings().setValue("recovery/original_file", window_body.current_file);
}

void save_as_file(WindowBody& window_body, const QString& filename) {
  Q_ASSERT(filename.isValidUtf16());

  XMLDocument document;
  populate_song_document(window_body, document);

  if (xmlSaveFile(filename.toStdString().c_str(), document.internal_pointer) <
      0) {
    QMessageBox::warning(&window_body, QObject::tr("Save error"),
                         QObject::tr("Failed to save file"));
    return;
  }

  window_body.current_file = filename;

  window_body.undo_stack.setClean();
  remove_recovery_file();
}

namespace {

auto check_xml_document(QWidget& parent, XMLDocument& document) -> bool {
  if (document.internal_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("XML error"),
                         QObject::tr("Invalid XML file"));
    return false;
  }
  return true;
}

auto maybe_read_xml_file(const QString& filename) -> XMLDocument {
  return XMLDocument(xmlReadFile(filename.toStdString().c_str(), nullptr, 0));
}

// loading a file replaces song.chords wholesale, which would leave
// pitched_notes_model/unpitched_notes_model pointing at destroyed Chord
// members if the switch table was drilled into a chord's notes (mirrors the
// reset that replace_table performs when the user navigates back to chords
// manually)
void reset_switch_table_to_chords(SwitchColumn& switch_column) {
  auto& switch_table = switch_column.switch_table;
  switch_table.pitched_notes_model.set_rows_pointer();
  switch_table.unpitched_notes_model.set_rows_pointer();
  switch_table.delegate.current_row_type = RowType::chord_type;
  set_model(switch_table, switch_table.chords_model);
  switch_column.editing_text.setText(SwitchColumn::tr("Chords"));
}

auto xml_to_double(const xmlNode& element) -> double {
  // std::stod uses the current C locale; QString::toDouble is always
  // locale-independent, so this matches set_xml_double above
  bool was_ok = false;
  const auto value =
      QString::fromStdString(get_content(element)).toDouble(&was_ok);
  Q_ASSERT(was_ok);
  return value;
}

}  // namespace

auto validate_against_schema(XMLValidator& validator, XMLDocument& document)
    -> int {
  return xmlSchemaValidateDoc(validator.context.internal_pointer,
                              document.internal_pointer);
}

auto open_file(WindowBody& window_body, const QString& filename) -> bool {
  Q_ASSERT(filename.isValidUtf16());
  auto& undo_stack = window_body.undo_stack;
  auto& spin_boxes = window_body.controls_column.spin_boxes;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& chords_model = switch_table.chords_model;
  auto& unpitched_voices_model = switch_table.unpitched_voices_model;
  auto& pitched_voices_model = switch_table.pitched_voices_model;

  auto document = maybe_read_xml_file(filename);
  if (!check_xml_document(window_body, document)) {
    return false;
  }

  static XMLValidator song_validator("song.xsd");
  if (validate_against_schema(song_validator, document) != 0) {
    QMessageBox::warning(&window_body, QObject::tr("Validation Error"),
                         QObject::tr("Invalid song file"));
    return false;
  }

  auto& song_node = get_root(document);

  // parse into scratch lists and validate voice names/references before
  // touching the current song, so a file that fails validation can't wipe
  // out the switch table's contents (see open_file's history for the bug
  // this avoids: clearing/repopulating first meant a rejected file still
  // destroyed whatever was previously open, with no way to undo back to it)
  QList<Chord> new_chords;
  QList<PitchedVoice> new_pitched_voices;
  QList<UnpitchedVoice> new_unpitched_voices;
  // notes refer to voices by name, so parse every voice before any chord
  xmlNode* chords_pointer = nullptr;
  for (auto& field_node : get_xml_children(song_node)) {
    const auto name = get_xml_name(field_node);
    if (name == "chords") {
      chords_pointer = &field_node;
    } else if (name == "pitched_voices") {
      xml_to_rows(new_pitched_voices, field_node, new_pitched_voices,
                  new_unpitched_voices);
    } else if (name == "unpitched_voices") {
      xml_to_rows(new_unpitched_voices, field_node, new_pitched_voices,
                  new_unpitched_voices);
    }
  }
  if (chords_pointer != nullptr) {
    xml_to_rows(new_chords, get_reference(chords_pointer), new_pitched_voices,
                new_unpitched_voices);
  }

  auto names_and_voices_ok =
      check_duplicate_or_empty_voice_names(window_body, new_pitched_voices) &&
      check_duplicate_or_empty_voice_names(window_body, new_unpitched_voices);
  if (names_and_voices_ok) {
    for (auto chord_number = 0; chord_number < new_chords.size();
         chord_number = chord_number + 1) {
      const auto& chord = new_chords.at(chord_number);
      if (!check_note_voices(window_body, chord.pitched_notes,
                             new_pitched_voices, chord_number) ||
          !check_note_voices(window_body, chord.unpitched_notes,
                             new_unpitched_voices, chord_number)) {
        names_and_voices_ok = false;
        break;
      }
    }
  }

  if (!names_and_voices_ok) {
    return false;
  }

  reset_switch_table_to_chords(window_body.switch_column);
  clear_rows(chords_model);
  clear_rows(pitched_voices_model);
  clear_rows(unpitched_voices_model);

  for (auto& field_node : get_xml_children(song_node)) {
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
      chords_model.insert_xml_rows(0, field_node, new_pitched_voices,
                                   new_unpitched_voices);
    } else if (name == "pitched_voices") {
      pitched_voices_model.insert_xml_rows(0, field_node, new_pitched_voices,
                                           new_unpitched_voices);
    } else {
      Q_ASSERT(name == "unpitched_voices");
      unpitched_voices_model.insert_xml_rows(0, field_node, new_pitched_voices,
                                             new_unpitched_voices);
    }
  }

  window_body.current_file = filename;

  clear_and_clean(undo_stack);
  remove_recovery_file();
  return true;
}

auto maybe_restore_recovery(WindowBody& window_body) -> bool {
  const auto recovery_file = get_recovery_file_path();
  if (!QFile::exists(recovery_file)) {
    return false;
  }

  if (QMessageBox::question(
          &window_body, WindowBody::tr("Recover unsaved work"),
          WindowBody::tr("Justly didn't close properly last time. Restore "
                         "the unsaved work from your last session?")) !=
      QMessageBox::Yes) {
    remove_recovery_file();
    return false;
  }

  const auto original_file =
      QSettings().value("recovery/original_file").toString();

  // open_file always points current_file at whatever filename it's given,
  // and removes recovery.xml as a side effect once loaded
  if (!open_file(window_body, recovery_file)) {
    return false;
  }
  window_body.current_file = original_file;

  // the recovered content was never saved, so mark it dirty even though
  // open_file's normal load path leaves the undo stack clean
  window_body.undo_stack.resetClean();
  return true;
}

void connect_recovery_timer(WindowBody& window_body) {
  static const auto RECOVERY_DEBOUNCE_MILLISECONDS = 5000;

  auto& recovery_timer = window_body.recovery_timer;
  auto& undo_stack = window_body.undo_stack;

  recovery_timer.setSingleShot(true);

  QObject::connect(&undo_stack, &QUndoStack::indexChanged, &recovery_timer,
                   [&recovery_timer]() -> auto {
                     recovery_timer.start(RECOVERY_DEBOUNCE_MILLISECONDS);
                   });
  QObject::connect(&recovery_timer, &QTimer::timeout, &window_body,
                   [&window_body]() -> auto {
                     if (window_body.undo_stack.isClean()) {
                       remove_recovery_file();
                     } else {
                       write_recovery_file(window_body);
                     }
                   });
}

namespace {

auto get_interval(const int midi_interval, const int septimal_quartertones = 0)
    -> Interval {
  // Johnston's 7 lowers by 36/35, taking a 9/5 minor seventh to a 7/4
  // harmonic seventh; his el raises by the same amount
  static const auto SEPTIMAL_QUARTERTONE = Rational(36, 35);
  const auto [octave, degree] = get_octave_degree(midi_interval);
  auto ratio = get_just_scale()[degree].ratio;
  if (septimal_quartertones > 0) {
    ratio = ratio * SEPTIMAL_QUARTERTONE;
  } else if (septimal_quartertones < 0) {
    ratio = ratio / SEPTIMAL_QUARTERTONE;
  }
  return Interval(ratio, octave);
}

auto get_max_duration(const QList<MusicXMLNote>& notes) -> int {
  if (notes.empty()) {
    return 0;
  }
  return std::ranges::max_element(notes,
                                  [](const MusicXMLNote& first_note,
                                     const MusicXMLNote& second_note) -> auto {
                                    return first_note.duration <
                                           second_note.duration;
                                  })
      ->duration;
}

void add_chord(ChordsModel& chords_model, const MusicXMLChord& parse_chord,
               const int measure_number, const int key, const int last_midi_key,
               const int song_divisions, const int time_delta) {
  // voices are already in the song, under their deduplicated names
  const auto& song = chords_model.song;
  Chord new_chord;
  new_chord.beats = Rational(time_delta, song_divisions);
  new_chord.interval = get_interval(key - last_midi_key);
  new_chord.words = QString::number(measure_number);
  auto& unpitched_notes = new_chord.unpitched_notes;
  for (const auto& parse_unpitched_note : parse_chord.unpitched_notes) {
    UnpitchedNote new_note;
    new_note.beats = Rational(parse_unpitched_note.duration, song_divisions);
    new_note.words = parse_unpitched_note.words;
    new_note.voice_name =
        song.unpitched_voices.at(parse_unpitched_note.voice_number).name;
    unpitched_notes.push_back(std::move(new_note));
  }
  auto& pitched_notes = new_chord.pitched_notes;
  for (const auto& parse_pitched_note : parse_chord.pitched_notes) {
    PitchedNote new_note;
    new_note.beats = Rational(parse_pitched_note.duration, song_divisions);
    new_note.words = parse_pitched_note.words;
    new_note.interval = get_interval(parse_pitched_note.midi_number - key,
                                     parse_pitched_note.septimal_quartertones);
    new_note.voice_name =
        song.pitched_voices.at(parse_pitched_note.voice_number).name;
    pitched_notes.push_back(std::move(new_note));
  }
  chords_model.insert_row(chords_model.rowCount(QModelIndex()),
                          std::move(new_chord));
}

auto deduplicate_voice_names(QList<QString> voice_names) -> QList<QString> {
  QSet<QString> used_names;
  for (auto& voice_name : voice_names) {
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

auto maybe_read_compressed_musicxml_bytes(const QString& filename)
    -> QByteArray {
  const ZipArchive archive(filename);
  if (archive.internal_pointer == nullptr) {
    return {};
  }

  const auto container_bytes =
      read_zip_entry(archive, "META-INF/container.xml");
  if (container_bytes.isEmpty()) {
    return {};
  }

  const auto container_document = read_xml_document(container_bytes);
  if (container_document.internal_pointer == nullptr) {
    return {};
  }

  auto* rootfiles_pointer =
      maybe_get_xml_child(get_root(container_document), "rootfiles");
  auto* rootfile_pointer =
      rootfiles_pointer == nullptr
          ? nullptr
          : maybe_get_xml_child(get_reference(rootfiles_pointer), "rootfile");
  if (rootfile_pointer == nullptr) {
    return {};
  }

  const auto root_path =
      get_property(get_reference(rootfile_pointer), "full-path");

  return read_zip_entry(archive, root_path);
}

auto maybe_read_musicxml_document(const QString& filename) -> XMLDocument {
  if (filename.endsWith(".mxl", Qt::CaseInsensitive)) {
    return read_xml_document(maybe_read_compressed_musicxml_bytes(filename));
  }
  return maybe_read_xml_file(filename);
}

}  // namespace

auto import_musicxml(WindowBody& window_body, const QString& filename) -> bool {
  auto& undo_stack = window_body.undo_stack;
  auto& spin_boxes = window_body.controls_column.spin_boxes;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& chords_model = switch_table.chords_model;
  auto& pitched_voices_model = switch_table.pitched_voices_model;
  auto& unpitched_voices_model = switch_table.unpitched_voices_model;

  auto document = maybe_read_musicxml_document(filename);
  if (!check_xml_document(window_body, document)) {
    return false;
  }

  static XMLValidator musicxml_validator("musicxml.xsd");
  if (validate_against_schema(musicxml_validator, document) != 0) {
    QMessageBox::warning(&window_body, QObject::tr("Validation Error"),
                         QObject::tr("Invalid musicxml file"));
    return false;
  }

  auto& score_partwise = get_root(document);
  if (!node_is(score_partwise, "score-partwise")) {
    QMessageBox::warning(
        &window_body, QObject::tr("Partwise error"),
        QObject::tr("Justly only supports partwise musicxml scores"));
    return false;
  }

  auto maybe_parts = parse_musicxml(window_body, score_partwise);
  if (!maybe_parts.has_value()) {
    return false;
  }
  auto& parts = maybe_parts.value();

  for (auto& part : parts) {
    fill_in_accidentals(part);
    untranspose(part);
    combine_ties(part);
  }
  const auto song_divisions = get_song_divisions(parts);
  for (auto& part : parts) {
    normalize_divisions(part, song_divisions);
    unroll_repeats(part);
  }
  auto voice_names = assign_voices(parts);

  const auto chords_dict = get_chords(parts);
  if (chords_dict.empty()) {
    QMessageBox::warning(&window_body, QObject::tr("Empty MusicXML error"),
                         QObject::tr("No chords"));
    return false;
  }
  const auto midi_keys = get_midi_keys(parts);
  const auto measure_numbers = get_measure_numbers(parts);

  if (voice_names.unpitched.empty()) {
    // a file with no percussion/unpitched notes would otherwise leave
    // song.unpitched_voices completely empty, so manually inserting any
    // unpitched note afterward (which defaults to the first voice) would
    // reference a voice that doesn't exist
    voice_names.unpitched.push_back(QObject::tr("unpitched voice 1"));
  }

  reset_switch_table_to_chords(window_body.switch_column);
  clear_rows(chords_model);
  clear_rows(pitched_voices_model);
  clear_rows(unpitched_voices_model);
  add_imported_voices(pitched_voices_model,
                      deduplicate_voice_names(voice_names.pitched));
  add_imported_voices(unpitched_voices_model,
                      deduplicate_voice_names(voice_names.unpitched));

  auto last_midi_key = get_most_recent(midi_keys, chords_dict.firstKey(),
                                       DEFAULT_STARTING_MIDI);
  spin_boxes.starting_key_editor.setValue(
      midi_number_to_frequency(last_midi_key));

  for (auto iterator = chords_dict.cbegin(); iterator != chords_dict.cend();
       ++iterator) {
    const auto time = iterator.key();
    const auto& chord = iterator.value();
    const auto next_iterator = std::next(iterator);
    const auto midi_key =
        get_most_recent(midi_keys, time, DEFAULT_STARTING_MIDI);
    add_chord(chords_model, chord, get_most_recent(measure_numbers, time, 1),
              midi_key, last_midi_key, song_divisions,
              next_iterator == chords_dict.cend()
                  ? std::max(get_max_duration(chord.pitched_notes),
                             get_max_duration(chord.unpitched_notes))
                  : next_iterator.key() - time);
    last_midi_key = midi_key;
  }

  clear_and_clean(undo_stack);
  remove_recovery_file();
  return true;
}

void add_menu_action(QMenu& menu, QAction& action,
                     const QKeySequence::StandardKey key_sequence,
                     const bool enabled) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}
