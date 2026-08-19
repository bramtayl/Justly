#include "widgets/SongWidget.hpp"

auto get_property(xmlNode &node, const char *name) -> std::string {
  return xml_string_to_string(xmlGetProp(&node, c_string_to_xml_string(name)));
}

SongWidget::SongWidget() {
  row_layout.addWidget(&controls_column, 0, Qt::AlignTop);
  row_layout.addWidget(&switch_column, 0, Qt::AlignTop);
}

SongWidget::~SongWidget() { undo_stack.disconnect(); }

auto get_next_row(const SongWidget &song_widget) -> int {
  return get_only_range(song_widget.switch_column.switch_table).bottom() + 1;
}

void initialize_play(SongWidget &song_widget) {
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  initialize_playstate(
      song, player.play_state,
      fluid_sequencer_get_tick(player.sequencer.internal_pointer));

  auto &channel_schedules = player.channel_schedules;
  Q_ASSERT(channel_schedules.size() == NUMBER_OF_MIDI_CHANNELS);
  std::ranges::fill(channel_schedules, 0);
  player.percussion_channels.clear();
}

auto pick_channel_index(const QList<double> &channel_end_times) -> int {
  return static_cast<int>(
      std::distance(std::begin(channel_end_times),
                    std::ranges::min_element(channel_end_times)));
}

auto channel_is_free(QWidget &parent, const QList<double> &channel_end_times,
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

auto get_channel_number(QWidget &parent, Player &player, const Program &program,
                        const double current_time) -> std::optional<int> {
  if (!is_pitched_bank_number(program.bank_number)) {
    auto &percussion_channels = player.percussion_channels;
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

void play_note(Player &player, const int channel_number,
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

  fluid_event_control_change(event.internal_pointer, channel_number,
                             BREATH_ID, velocity);
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

void update_final_time(Player &player, const double new_final_time) {
  player.final_time = std::max(new_final_time, player.final_time);
}

void play_chords(SongWidget &song_widget, const int first_chord_number,
                 const int number_of_chords, const int wait_frames) {
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

auto can_discard_changes(SongWidget &song_widget) -> bool {
  return song_widget.undo_stack.isClean() ||
         QMessageBox::question(&song_widget, SongWidget::tr("Unsaved changes"),
                               SongWidget::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

auto get_gain(const SongWidget &song_widget) -> double {
  return fluid_synth_get_gain(song_widget.player.synth.internal_pointer);
}

void export_to_file(SongWidget &song_widget, const QString &output_file) {
  Q_ASSERT(output_file.isValidUtf16());
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  auto &settings = player.settings;
  auto &event = player.event;
  auto &sequencer = player.sequencer;
  auto &driver = player.driver;

  stop_playing(sequencer, event);

  driver.reset();

  set_fluid_string(settings, "audio.file.name",
                   output_file.toStdString().c_str());

  set_fluid_int(settings, "synth.lock-memory", 0);

  auto finished = false;
  const auto finished_timer_id = fluid_sequencer_register_client(
      player.sequencer.internal_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) -> auto {
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

  auto *const renderer_pointer =
      new_fluid_file_renderer(player.synth.internal_pointer);
  if (renderer_pointer == nullptr) {
    QMessageBox::warning(&player.parent, QObject::tr("Export error"),
                         QObject::tr("Cannot write to file"));
  } else {
    auto &renderer = get_reference(renderer_pointer);
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

auto get_pitched_midi_channels() -> const QList<int> & {
  static const auto channels = []() -> QList<int> {
    QList<int> result;
    std::ranges::copy_if(
        std::views::iota(0, NUMBER_OF_STANDARD_MIDI_CHANNELS),
        std::back_inserter(result),
        [](const int channel_number) -> auto {
          return channel_number != MIDI_PERCUSSION_CHANNEL;
        });
    return result;
  }();
  return channels;
}

void emit_note_events(QList<MidiTrackEvent> &track,
                      unsigned int channel_number,
                      unsigned int midi_number,
                      unsigned int velocity, double start_tick,
                      double end_tick) {
  track.push_back(MidiTrackEvent{
      .tick = start_tick,
      .tie_break = MIDI_EXPORT_NOTE_ON_TIE_BREAK,
      .info = NoteOnEventInfo{.channel_number = channel_number,
                              .midi_number = midi_number,
                              .velocity = velocity}});

  track.push_back(MidiTrackEvent{
      .tick = end_tick,
      .tie_break = MIDI_EXPORT_NOTE_OFF_TIE_BREAK,
      .info = NoteOffEventInfo{.channel_number = channel_number,
                               .midi_number = midi_number}});
}

void export_midi_to_file(SongWidget &song_widget, const QString &output_file) {
  Q_ASSERT(output_file.isValidUtf16());

  const auto &song = song_widget.song;
  const auto &pitched_voices = song.pitched_voices;
  const auto &unpitched_voices = song.unpitched_voices;
  const auto number_of_pitched_voices =
      static_cast<int>(pitched_voices.size());
  const auto number_of_unpitched_voices =
      static_cast<int>(unpitched_voices.size());

  QList<QList<MidiTrackEvent>> tracks(1 + number_of_pitched_voices +
                                      number_of_unpitched_voices);
  tracks[0].push_back(MidiTrackEvent{
      .tick = 0,
      .tie_break = 0,
      .info = TempoEventInfo{
          .microseconds_per_quarter = MIDI_MICROSECONDS_PER_QUARTER}});
  for (auto voice_number = 0; voice_number < number_of_pitched_voices;
       voice_number = voice_number + 1) {
    tracks[1 + voice_number].push_back(MidiTrackEvent{
        .tick = 0,
        .tie_break = 0,
        .info = TrackNameEventInfo{
            .name = pitched_voices.at(voice_number).name}});
  }
  for (auto voice_number = 0; voice_number < number_of_unpitched_voices;
       voice_number = voice_number + 1) {
    tracks[1 + number_of_pitched_voices + voice_number].push_back(
        MidiTrackEvent{
            .tick = 0,
            .tie_break = 0,
            .info = TrackNameEventInfo{
                .name = unpitched_voices.at(voice_number).name}});
  }

  const auto &pitched_channels = get_pitched_midi_channels();
  QList<double> pitched_channel_end_times(pitched_channels.size(), 0.0);

  // General MIDI has exactly one percussion channel, so two unpitched
  // voices with different programs can't both sound at the same tick --
  // a program change is channel-wide state, and only one can be in effect
  // when the resulting note-ons fire
  auto has_percussion_program = false;
  auto percussion_tick = 0.0;
  short percussion_preset_number = 0;

  for (const auto &event : get_piano_roll_events(song)) {
    const auto start_tick = event.start_time_ms;
    const auto end_tick = event.start_time_ms + event.duration_ms;

    // matches play_notes' velocity check exactly -- export aborts on the
    // same problems live playback would, rather than silently clamping
    // and producing a file with quieter notes than the user asked for
    const auto velocity = static_cast<int>(std::round(event.velocity));
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      if (event.is_pitched) {
        add_note_location<PitchedNote>(stream, event.chord_number,
                                       event.note_number);
      } else {
        add_note_location<UnpitchedNote>(stream, event.chord_number,
                                         event.note_number);
      }
      QMessageBox::warning(&song_widget, QObject::tr("Velocity error"), message);
      return;
    }

    if (event.is_pitched) {
      const auto frequency = event.frequency;
      static const auto minimum_frequency =
          midi_number_to_frequency(0 - QUARTER_STEP);
      if (frequency < minimum_frequency || frequency >= MAX_FREQUENCY) {
        QString message;
        QTextStream stream(&message);
        stream << QObject::tr("Frequency ")
               << QString::number(frequency, 'g', 3);
        add_note_location<PitchedNote>(stream, event.chord_number,
                                       event.note_number);
        stream << QObject::tr(" is out of MIDI export range");
        QMessageBox::warning(&song_widget, QObject::tr("Frequency error"),
                             message);
        return;
      }

      const auto midi_float = frequency_to_midi_number(frequency);
      const auto closest_midi = to_int(midi_float);
      const auto bend = to_int((midi_float - closest_midi +
                                ZERO_BEND_HALFSTEPS) *
                               BEND_PER_HALFSTEP);

      const auto channel_index = pick_channel_index(pitched_channel_end_times);
      if (!channel_is_free(song_widget, pitched_channel_end_times,
                           channel_index, start_tick)) {
        return;
      }
      const auto channel_number = pitched_channels.at(channel_index);
      const auto &program = get_voice_program(get_some_programs(true),
                                              pitched_voices,
                                              event.voice_number);
      pitched_channel_end_times[channel_index] =
          end_tick + program.release_milliseconds;

      auto &track = tracks[1 + event.voice_number];
      // no bank-select here: program.bank_number is this soundfont's own
      // private numbering (e.g. 17 for "Expr." variants), not a portable GM2
      // bank -- an unrecognized bank-select MSB is undefined behavior on
      // generic GM2 hardware, so the exported instrument intentionally falls
      // back to the plain bank-0 sibling everywhere except when reopened with
      // this exact soundfont
      track.push_back(MidiTrackEvent{
          .tick = start_tick,
          .tie_break = MIDI_EXPORT_PROGRAM_CHANGE_TIE_BREAK,
          .info = ProgramChangeEventInfo{
              .channel_number = static_cast<unsigned int>(channel_number),
              .program_number =
                  static_cast<unsigned int>(program.preset_number)}});

      const auto bend_14_bit = static_cast<unsigned int>(bend);
      track.push_back(MidiTrackEvent{
          .tick = start_tick,
          .tie_break = MIDI_EXPORT_PITCH_BEND_TIE_BREAK,
          .info = PitchBendEventInfo{
              .channel_number = static_cast<unsigned int>(channel_number),
              .bend_14_bit = bend_14_bit}});

      // mirrors play_note's live-playback behavior: single note dynamics
      // (see MS_Basic.sf3's "Expr." presets) read note volume from
      // the breath controller, not note-on velocity, so both must carry the
      // same value for expressive instruments to have correct dynamics
      track.push_back(MidiTrackEvent{
          .tick = start_tick,
          .tie_break = MIDI_EXPORT_BREATH_TIE_BREAK,
          .info = ControlChangeEventInfo{
              .channel_number = static_cast<unsigned int>(channel_number),
              .controller = BREATH_ID,
              .value = static_cast<unsigned int>(velocity)}});

      emit_note_events(track, static_cast<unsigned int>(channel_number),
                       static_cast<unsigned int>(closest_midi),
                       static_cast<unsigned int>(velocity), start_tick,
                       end_tick);
    } else {
      const auto &voice = unpitched_voices.at(event.voice_number);
      const auto &program = get_voice_program(get_some_programs(false),
                                              unpitched_voices,
                                              event.voice_number);

      if (has_percussion_program && start_tick == percussion_tick &&
          program.preset_number != percussion_preset_number) {
        QString message;
        QTextStream stream(&message);
        stream << QObject::tr("Percussion instrument ") << program.name;
        add_note_location<UnpitchedNote>(stream, event.chord_number,
                                         event.note_number);
        stream << QObject::tr(
            " starts at the same time as a different percussion instrument "
            "on the shared MIDI percussion channel");
        QMessageBox::warning(&song_widget, QObject::tr("Percussion channel conflict"),
                             message);
        return;
      }
      has_percussion_program = true;
      percussion_tick = start_tick;
      percussion_preset_number = program.preset_number;

      auto &track =
          tracks[1 + number_of_pitched_voices + event.voice_number];
      track.push_back(MidiTrackEvent{
          .tick = start_tick,
          .tie_break = MIDI_EXPORT_BANK_SELECT_TIE_BREAK,
          .info = ControlChangeEventInfo{
              .channel_number =
                  static_cast<unsigned int>(MIDI_PERCUSSION_CHANNEL),
              .controller = MIDI_BANK_SELECT_MSB_CONTROLLER,
              .value = GM2_PERCUSSION_BANK_SELECT_MSB}});

      track.push_back(MidiTrackEvent{
          .tick = start_tick,
          .tie_break = MIDI_EXPORT_PROGRAM_CHANGE_TIE_BREAK,
          .info = ProgramChangeEventInfo{
              .channel_number =
                  static_cast<unsigned int>(MIDI_PERCUSSION_CHANNEL),
              .program_number =
                  static_cast<unsigned int>(program.preset_number)}});

      emit_note_events(track,
                       static_cast<unsigned int>(MIDI_PERCUSSION_CHANNEL),
                       static_cast<unsigned int>(voice.midi_number),
                       static_cast<unsigned int>(velocity), start_tick,
                       end_tick);
    }
  }

  QFile file(output_file);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(&song_widget, QObject::tr("File error"),
                         QObject::tr("Cannot open file for writing"));
    return;
  }
  QByteArray output;

  QByteArray header;
  append_be16(header, MIDI_FORMAT_MULTI_TRACK);
  append_be16(header, static_cast<unsigned int>(tracks.size()));
  append_be16(header, MIDI_TICKS_PER_QUARTER);
  append_chunk(output, "MThd", header);

  for (const auto &track_events : tracks) {
    auto sorted_events = track_events;
    std::ranges::stable_sort(
        sorted_events, [](const MidiTrackEvent &first,
                          const MidiTrackEvent &second) -> auto {
          if (first.tick != second.tick) {
            return first.tick < second.tick;
          }
          return first.tie_break < second.tie_break;
        });
    QByteArray track_data;
    auto previous_tick = 0U;
    for (const auto &event : sorted_events) {
      const auto tick = static_cast<unsigned int>(std::llround(event.tick));
      append_variable_length(track_data, tick - previous_tick);
      event.write(track_data);
      previous_tick = tick;
    }
    append_variable_length(track_data, 0);
    append_meta_event(track_data, MIDI_END_OF_TRACK_META_TYPE, {});
    append_chunk(output, "MTrk", track_data);
  }

  file.write(output);
}

void set_xml_double(xmlNode &node, const char *const field_name,
                    double value) {
  // std::to_string uses the current C locale, which can use a comma for the
  // decimal separator; QString::number is always locale-independent,
  // matching the xs:decimal lexical form required by song.xsd. (std::to_chars
  // would also work, but Apple's libc++ only supports the floating-point
  // overloads when targeting very recent macOS versions, and this project
  // deliberately supports macOS back to 12.0.)
  static const auto double_digits = std::numeric_limits<double>::max_digits10;
  set_xml_string(
      node, field_name,
      QString::number(value, 'g', double_digits).toStdString());
}

void populate_song_document(SongWidget &song_widget, XMLDocument &document) {
  const auto &song = song_widget.song;

  auto &song_node = make_root(document, "song");

  set_xml_double(song_node, "gain", get_gain(song_widget));
  set_xml_double(song_node, "starting_key", song.starting_key);
  set_xml_double(song_node, "starting_tempo", song.starting_tempo);
  set_xml_double(song_node, "starting_velocity", song.starting_velocity);

  maybe_set_xml_rows(song_node, "chords", song.chords);
  maybe_set_xml_rows(song_node, "pitched_voices", song.pitched_voices);
  maybe_set_xml_rows(song_node, "unpitched_voices", song.unpitched_voices);
}

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

void write_recovery_file(SongWidget &song_widget) {
  XMLDocument document;
  populate_song_document(song_widget, document);
  if (xmlSaveFile(get_recovery_file_path().toStdString().c_str(),
                  document.internal_pointer) < 0) {
    // leave any pre-existing recovery.xml and its original_file setting
    // alone rather than pointing them at content that was never written
    return;
  }

  // remembers where the recovered content should be saved back to, since
  // open_file (used to reload recovery.xml) always overwrites current_file
  // with whatever path it's given
  QSettings().setValue("recovery/original_file", song_widget.current_file);
}

void save_as_file(SongWidget &song_widget, const QString &filename) {
  Q_ASSERT(filename.isValidUtf16());

  XMLDocument document;
  populate_song_document(song_widget, document);

  if (xmlSaveFile(filename.toStdString().c_str(), document.internal_pointer) <
      0) {
    QMessageBox::warning(&song_widget, QObject::tr("Save error"),
                         QObject::tr("Failed to save file"));
    return;
  }

  song_widget.current_file = filename;

  song_widget.undo_stack.setClean();
  remove_recovery_file();
}

auto check_xml_document(QWidget &parent, XMLDocument &document) -> bool {
  if (document.internal_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("XML error"),
                         QObject::tr("Invalid XML file"));
    return false;
  }
  return true;
}

auto maybe_read_xml_file(const QString &filename) -> XMLDocument {
  return XMLDocument(xmlReadFile(filename.toStdString().c_str(), nullptr, 0));
}

auto get_int_or_warn(QWidget &parent, const std::string &content,
                     const QString &title, const QString &message)
    -> std::optional<int> {
  auto maybe_int = string_to_maybe_int(content);
  if (!maybe_int.has_value()) {
    QMessageBox::warning(&parent, title, message);
  }
  return maybe_int;
}

auto get_int_or_warn(QWidget &parent, const xmlNode &element,
                     const QString &title, const QString &message)
    -> std::optional<int> {
  return get_int_or_warn(parent, get_content(element), title, message);
}

void reset_switch_table_to_chords(SwitchColumn &switch_column) {
  auto &switch_table = switch_column.switch_table;
  switch_table.pitched_notes_model.set_rows_pointer();
  switch_table.unpitched_notes_model.set_rows_pointer();
  switch_table.delegate.current_row_type = RowType::chord_type;
  set_model(switch_table, switch_table.chords_model);
  switch_column.editing_text.setText(SwitchColumn::tr("Chords"));
}

auto xml_to_double(const xmlNode &element) -> double {
  // std::stod uses the current C locale; QString::toDouble is always
  // locale-independent, so this matches set_xml_double above
  bool was_ok = false;
  const auto value =
      QString::fromStdString(get_content(element)).toDouble(&was_ok);
  Q_ASSERT(was_ok);
  return value;
}

auto validate_against_schema(XMLValidator &validator, XMLDocument &document) -> int {
  return xmlSchemaValidateDoc(validator.context.internal_pointer,
                              document.internal_pointer);
}

auto open_file(SongWidget &song_widget, const QString &filename) -> bool {

  Q_ASSERT(filename.isValidUtf16());
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &chords_model = switch_table.chords_model;
  auto &unpitched_voices_model = switch_table.unpitched_voices_model;
  auto &pitched_voices_model = switch_table.pitched_voices_model;

  auto document = maybe_read_xml_file(filename);
  if (!check_xml_document(song_widget, document)) {
    return false;
  }

  static XMLValidator song_validator("song.xsd");
  if (validate_against_schema(song_validator, document) != 0) {
    QMessageBox::warning(&song_widget, QObject::tr("Validation Error"),
                         QObject::tr("Invalid song file"));
    return false;
  }

  auto &song_node = get_root(document);

  // parse into scratch lists and validate voice names/references before
  // touching the current song, so a file that fails validation can't wipe
  // out the switch table's contents (see open_file's history for the bug
  // this avoids: clearing/repopulating first meant a rejected file still
  // destroyed whatever was previously open, with no way to undo back to it)
  QList<Chord> new_chords;
  QList<PitchedVoice> new_pitched_voices;
  QList<UnpitchedVoice> new_unpitched_voices;
  for (auto *field_pointer = xmlFirstElementChild(&song_node);
       field_pointer != nullptr;
       field_pointer = xmlNextElementSibling(field_pointer)) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "chords") {
      xml_to_rows(new_chords, field_node);
    } else if (name == "pitched_voices") {
      xml_to_rows(new_pitched_voices, field_node);
    } else if (name == "unpitched_voices") {
      xml_to_rows(new_unpitched_voices, field_node);
    }
  }

  auto names_and_voices_ok =
      check_duplicate_or_empty_voice_names(song_widget, new_pitched_voices) &&
      check_duplicate_or_empty_voice_names(song_widget, new_unpitched_voices);
  if (names_and_voices_ok) {
    const auto number_of_pitched_voices =
        static_cast<int>(new_pitched_voices.size());
    const auto number_of_unpitched_voices =
        static_cast<int>(new_unpitched_voices.size());
    for (auto chord_number = 0; chord_number < new_chords.size();
        chord_number = chord_number + 1) {
      const auto &chord = new_chords.at(chord_number);
      if (!check_note_voices(song_widget, chord.pitched_notes,
                             number_of_pitched_voices, chord_number) ||
          !check_note_voices(song_widget, chord.unpitched_notes,
                             number_of_unpitched_voices, chord_number)) {
        names_and_voices_ok = false;
        break;
      }
    }
  }

  if (!names_and_voices_ok) {
    return false;
  }

  reset_switch_table_to_chords(song_widget.switch_column);
  clear_rows(chords_model);
  clear_rows(pitched_voices_model);
  clear_rows(unpitched_voices_model);

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
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }

  song_widget.current_file = filename;

  clear_and_clean(undo_stack);
  remove_recovery_file();
  return true;
}

auto maybe_restore_recovery(SongWidget &song_widget) -> bool {
  const auto recovery_file = get_recovery_file_path();
  if (!QFile::exists(recovery_file)) {
    return false;
  }

  if (QMessageBox::question(
          &song_widget, SongWidget::tr("Recover unsaved work"),
          SongWidget::tr("Justly didn't close properly last time. Restore "
                         "the unsaved work from your last session?")) !=
      QMessageBox::Yes) {
    remove_recovery_file();
    return false;
  }

  const auto original_file =
      QSettings().value("recovery/original_file").toString();

  // open_file always points current_file at whatever filename it's given,
  // and removes recovery.xml as a side effect once loaded
  if (!open_file(song_widget, recovery_file)) {
    return false;
  }
  song_widget.current_file = original_file;

  // the recovered content was never saved, so mark it dirty even though
  // open_file's normal load path leaves the undo stack clean
  song_widget.undo_stack.resetClean();
  return true;
}

void connect_recovery_timer(SongWidget &song_widget) {
  auto &recovery_timer = song_widget.recovery_timer;
  auto &undo_stack = song_widget.undo_stack;

  recovery_timer.setSingleShot(true);

  QObject::connect(&undo_stack, &QUndoStack::indexChanged, &recovery_timer,
                   [&recovery_timer]() -> auto {
                     recovery_timer.start(RECOVERY_DEBOUNCE_MILLISECONDS);
                   });
  QObject::connect(&recovery_timer, &QTimer::timeout, &song_widget,
                   [&song_widget]() -> auto {
                     if (song_widget.undo_stack.isClean()) {
                       remove_recovery_file();
                     } else {
                       write_recovery_file(song_widget);
                     }
                   });
}

auto node_is(const xmlNode &node, const char *name) -> bool {
  return get_xml_name(node) == name;
}

auto maybe_get_xml_child(xmlNode &node, const char *name) -> xmlNode * {
  auto *child_pointer = xmlFirstElementChild(&node);
  while (child_pointer != nullptr) {
    if (node_is(get_reference(child_pointer), name)) {
      return child_pointer;
    }
    child_pointer = xmlNextElementSibling(child_pointer);
  }
  return nullptr;
}

auto get_xml_child(xmlNode &node, const char *name) -> xmlNode & {
  return get_reference(maybe_get_xml_child(node, name));
}

auto get_duration(QWidget &parent, xmlNode &measure_element)
    -> std::optional<int> {
  auto &duration_element = get_xml_child(measure_element, "duration");
  if (!xml_content_is_integer(duration_element)) {
    QMessageBox::warning(
        &parent, QObject::tr("Duration error"),
        QObject::tr("Fractional durations are not supported"));
    return std::nullopt;
  }
  return get_int_or_warn(parent, duration_element,
                         QObject::tr("Duration error"),
                         QObject::tr("Duration is out of range"));
}

auto get_interval(const int midi_interval) -> Interval {
  const auto [octave, degree] = get_octave_degree(midi_interval);
  static const QList<Rational> scale = {
      Rational(1, 1), Rational(16, 15), Rational(9, 8),   Rational(6, 5),
      Rational(5, 4), Rational(4, 3),   Rational(45, 32), Rational(3, 2),
      Rational(8, 5), Rational(5, 3),   Rational(9, 5),   Rational(15, 8)};
  return Interval(scale[degree], octave);
}

auto get_max_duration(const QList<MusicXMLNote> &notes) -> int {
  if (notes.empty()) {
    return 0;
  }
  return std::ranges::max_element(notes,
                                  [](const MusicXMLNote &first_note,
                                     const MusicXMLNote &second_note) -> auto {
                                    return first_note.duration <
                                           second_note.duration;
                                  })
      ->duration;
}

void add_chord(ChordsModel &chords_model, const MusicXMLChord &parse_chord,
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

void add_note(MusicXMLChord &chord, MusicXMLNote note, bool is_pitched) {
  (is_pitched ? chord.pitched_notes : chord.unpitched_notes)
      .push_back(std::move(note));
}

void add_note_and_maybe_chord(QMap<int, MusicXMLChord> &chords_dict,
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

auto get_most_recent(MostRecentIterator &iterator, const int time) -> int {
  auto &iterator_state = iterator.state;
  auto &iterator_value = iterator.value;
  const auto &iterator_end = iterator.end;
  while (iterator_state != iterator_end && iterator_state.key() <= time) {
    iterator_value = iterator_state.value();
    ++iterator_state;
  }
  return iterator_value;
}

void reset(TimeIterator &iterator) {
  iterator.state = iterator.dict.begin();
  iterator.last_change_time = 0;
  iterator.next_change_divisions_time = 0;
  iterator.time_per_division = 1;
}

auto compute_measure_expansion(const QList<MeasureRepeatInfo> &measure_infos)
    -> QList<std::pair<int, int>> {
  QList<std::pair<int, int>> expansion;
  const auto number_of_measures = static_cast<int>(measure_infos.size());
  auto repeat_start_index = -1;
  auto block_start_index = 0;

  const auto flush = [&](const int first_index, const int last_index) -> auto {
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
      const auto start_index = repeat_start_index == -1 ? block_start_index
                                                          : repeat_start_index;
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

auto get_time_and_time_per_division(TimeIterator &iterator,
                                    const int check_divisions_time)
    -> std::tuple<int, int> {
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

auto deduplicate_voice_names(QList<QString> voice_names) -> QList<QString> {
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

auto maybe_read_compressed_musicxml_bytes(const QString &filename) -> QByteArray {
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

  auto *rootfiles_pointer =
      maybe_get_xml_child(get_root(container_document), "rootfiles");
  auto *rootfile_pointer =
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

auto maybe_read_musicxml_document(const QString &filename) -> XMLDocument {
  if (filename.endsWith(".mxl", Qt::CaseInsensitive)) {
    return read_xml_document(maybe_read_compressed_musicxml_bytes(filename));
  }
  return maybe_read_xml_file(filename);
}

auto import_musicxml(SongWidget &song_widget, const QString &filename) -> bool {
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &chords_model = switch_table.chords_model;
  auto &pitched_voices_model = switch_table.pitched_voices_model;
  auto &unpitched_voices_model = switch_table.unpitched_voices_model;

  auto document = maybe_read_musicxml_document(filename);
  if (!check_xml_document(song_widget, document)) {
    return false;
  }

  static XMLValidator musicxml_validator("musicxml.xsd");
  if (validate_against_schema(musicxml_validator, document) != 0) {
    QMessageBox::warning(&song_widget, QObject::tr("Validation Error"),
                         QObject::tr("Invalid musicxml file"));
    return false;
  }

  // Get root_pointer element
  auto &score_partwise = get_root(document);
  if (!node_is(score_partwise, "score-partwise")) {
    QMessageBox::warning(
        &song_widget, QObject::tr("Partwise error"),
        QObject::tr("Justly only supports partwise musicxml scores"));
    return false; // endpoint
  }

  // Get part-list
  QMap<std::string, PartInfo> part_info_dict;

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

      QMap<QString, MusicXMLNote> tied_notes;
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
                const auto maybe_fifths = get_int_or_warn(
                    song_widget, get_xml_child(attribute_element, "fifths"),
                    QObject::tr("Key error"),
                    QObject::tr("Fifths value is out of range"));
                if (!maybe_fifths.has_value()) {
                  return false; // endpoint
                }
                const auto [octave, degree] = get_octave_degree(
                    FIFTH_HALFSTEPS * maybe_fifths.value());
                part_midi_keys_dict[current_time] = MIDDLE_C_MIDI + degree;
              } else if (attribute_name == "divisions") {
                if (!xml_content_is_integer(attribute_element)) {
                  QMessageBox::warning(
                      &song_widget, QObject::tr("Divisions error"),
                      QObject::tr("Fractional divisions are not supported"));
                  return false; // endpoint
                }
                const auto maybe_divisions = get_int_or_warn(
                    song_widget, attribute_element,
                    QObject::tr("Divisions error"),
                    QObject::tr("Divisions value is out of range"));
                if (!maybe_divisions.has_value()) {
                  return false; // endpoint
                }
                const auto new_divisions = maybe_divisions.value();
                Q_ASSERT(new_divisions > 0);
                song_divisions = std::lcm(song_divisions, new_divisions);
                part_divisions_dict[current_time] = new_divisions;
              } else if (attribute_name == "transpose") {
                auto &chromatic_element =
                    get_xml_child(attribute_element, "chromatic");
                if (!xml_content_is_integer(chromatic_element)) {
                  QMessageBox::warning(
                      &song_widget, QObject::tr("Transpose error"),
                      QObject::tr(
                          "Microtonal transpositions are not supported"));
                  return false; // endpoint
                }
                const auto maybe_chromatic = get_int_or_warn(
                    song_widget, chromatic_element,
                    QObject::tr("Transpose error"),
                    QObject::tr("Chromatic value is out of range"));
                if (!maybe_chromatic.has_value()) {
                  return false; // endpoint
                }
                const auto chromatic_semitones = maybe_chromatic.value();
                auto octave_change_octaves = 0;
                auto *transpose_field_pointer =
                    xmlFirstElementChild(&attribute_element);
                while (transpose_field_pointer != nullptr) {
                  auto &transpose_field =
                      get_reference(transpose_field_pointer);
                  if (node_is(transpose_field, "octave-change")) {
                    const auto maybe_octave_change = get_int_or_warn(
                        song_widget, transpose_field,
                        QObject::tr("Transpose error"),
                        QObject::tr("Octave change value is out of range"));
                    if (!maybe_octave_change.has_value()) {
                      return false; // endpoint
                    }
                    octave_change_octaves = maybe_octave_change.value();
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
                    if (!xml_content_is_integer(pitch_field)) {
                      QMessageBox::warning(
                          &song_widget, QObject::tr("Pitch error"),
                          QObject::tr(
                              "Microtonal pitches are not supported"));
                      return false; // endpoint
                    }
                    const auto maybe_alter = get_int_or_warn(
                        song_widget, pitch_field, QObject::tr("Pitch error"),
                        QObject::tr("Alter value is out of range"));
                    if (!maybe_alter.has_value()) {
                      return false; // endpoint
                    }
                    alter = maybe_alter.value();
                  }
                  pitch_field_pointer =
                      xmlNextElementSibling(pitch_field_pointer);
                }
                midi_number = midi_degree + alter +
                              octave_number * HALFSTEPS_PER_OCTAVE + C_0_MIDI;
              } else if (name == "duration") {
                if (!xml_content_is_integer(note_field)) {
                  QMessageBox::warning(
                      &song_widget, QObject::tr("Note duration error"),
                      QObject::tr(
                          "Fractional note durations are not supported"));
                  return false; // endpoint
                }
                const auto maybe_duration = get_int_or_warn(
                    song_widget, note_field,
                    QObject::tr("Note duration error"),
                    QObject::tr("Note duration is out of range"));
                if (!maybe_duration.has_value()) {
                  return false; // endpoint
                }
                note_duration = maybe_duration.value();
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
              return false; // endpoint
            }
            if (new_chord) {
              chord_start_time = current_time;
              current_time += note_duration;
            }
            if (!is_rest) {
              QString voice_key;
              QTextStream voice_key_stream(&voice_key);
              voice_key_stream << QString::fromStdString(part_id) << ":"
                               << QString::fromStdString(instrument_id);
              // a tie only ever connects notes within the same voice, so an
              // in-progress tie must be looked up by voice as well as pitch
              // -- otherwise two simultaneous voices (e.g. two instruments
              // in one part, or an unresolved tie carried over from an
              // earlier part) tying the same pitch clobber each other's
              // still-open note
              const auto tied_note_key =
                  voice_key + ":" + QString::number(midi_number);
              if (tie_end && !tied_notes.contains(tied_note_key)) {
                // no matching tie-start -- the schema doesn't require ties
                // to be well-formed, so a malformed or hand-edited file can
                // have an orphan tie-stop; fall back to treating this as an
                // unstarted note rather than dereferencing a missing entry
                tie_end = false;
              }
              if (tie_end) {
                const auto tied_notes_iterator =
                    tied_notes.find(tied_note_key);
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
                const auto voice_name = instrument_name.isEmpty()
                                            ? part_info.part_name
                                            : instrument_name;
                const auto found_voice_number = voice_numbers.find(voice_key);
                if (found_voice_number != voice_numbers.end()) {
                  new_note.voice_number = found_voice_number.value();
                } else {
                  new_note.voice_number = static_cast<int>(voice_names.size());
                  voice_numbers[voice_key] = new_note.voice_number;
                  voice_names.push_back(voice_name);
                }
                if (tie_start) { // also not tie end
                  tied_notes[tied_note_key] = std::move(new_note);
                } else { // not tie start or end
                  add_note_and_maybe_chord(part_chords_dict,
                                           std::move(new_note), is_pitched);
                }
              }
            }
          } else if (measure_element_name == "backup") {
            const auto duration = get_duration(song_widget, measure_element);
            if (!duration.has_value()) {
              return false; // endpoint
            }
            current_time -= duration.value();
            chord_start_time = current_time;
          } else if (measure_element_name == "forward") {
            const auto duration = get_duration(song_widget, measure_element);
            if (!duration.has_value()) {
              return false; // endpoint
            }
            current_time += duration.value();
            chord_start_time = current_time;
          } else if (measure_element_name == "barline") {
            // records forward/backward repeats and first/second-ending
            // brackets onto the current measure, so the raw per-part
            // timeline can be unrolled below
            auto *barline_child_pointer =
                xmlFirstElementChild(&measure_element);
            while (barline_child_pointer != nullptr) {
              auto &child = get_reference(barline_child_pointer);
              if (node_is(child, "repeat")) {
                const auto direction = get_property(child, "direction");
                if (direction == "forward") {
                  measure_info.has_forward_repeat = true;
                } else if (direction == "backward") {
                  measure_info.has_backward_repeat = true;
                  auto *const times_property = xmlGetProp(
                      &child, c_string_to_xml_string("times"));
                  const auto times_text =
                      times_property == nullptr
                          ? std::string()
                          : xml_string_to_string(times_property);
                  if (times_text.empty()) {
                    measure_info.repeat_times = DEFAULT_REPEAT_TIMES;
                  } else {
                    const auto maybe_times = get_int_or_warn(
                        song_widget, times_text, QObject::tr("Repeat error"),
                        QObject::tr("Repeat times is out of range"));
                    if (!maybe_times.has_value()) {
                      return false; // endpoint
                    }
                    measure_info.repeat_times = maybe_times.value();
                  }
                }
              } else if (node_is(child, "ending")) {
                if (get_property(child, "type") == "start") {
                  QList<int> ending_numbers;
                  const auto numbers_text = QString::fromStdString(
                      get_property(child, "number"));
                  for (const auto &token :
                       numbers_text.split(',', Qt::SkipEmptyParts)) {
                    bool is_number = false;
                    const auto number = token.trimmed().toInt(&is_number);
                    if (is_number) {
                      ending_numbers.push_back(number);
                    }
                  }
                  for (const auto number : ending_numbers) {
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
              barline_child_pointer =
                  xmlNextElementSibling(barline_child_pointer);
            }
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
    return false; // endpoint
  }

  if (unpitched_voice_names.empty()) {
    // a file with no percussion/unpitched notes would otherwise leave
    // song.unpitched_voices completely empty, so manually inserting any
    // unpitched note afterward (which defaults its voice_number to 0)
    // would reference a voice that doesn't exist
    unpitched_voice_names.push_back(QObject::tr("unpitched voice 1"));
  }

  reset_switch_table_to_chords(song_widget.switch_column);
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
  remove_recovery_file();
  return true;
}

void add_menu_action(QMenu &menu, QAction &action,
                     const QKeySequence::StandardKey key_sequence,
                     const bool enabled) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}
