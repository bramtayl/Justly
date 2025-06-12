#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QIterator>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/Qt>
#include <QtCore/QtGlobal>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <fluidsynth.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <numeric>
#include <string>
#include <tuple>
#include <utility>

#include "rows/Chord.hpp"
#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "iterators/MostRecentIterator.hpp"
#include "iterators/TimeIterator.hpp"
#include "models/ChordsModel.hpp"
#include "musicxml/MusicXMLChord.hpp"
#include "musicxml/MusicXMLNote.hpp"
#include "musicxml/PartInfo.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/Row.hpp"
#include "rows/UnpitchedNote.hpp"
#include "sound/PlayState.hpp"
#include "sound/Player.hpp"
#include "tables/ChordsTable.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/XMLValidator.hpp"

template <RowInterface SubRow> struct RowsModel;

static const auto FIFTH_HALFSTEPS = 7;
static const auto MIDDLE_C_MIDI = 60;
static const auto START_END_MILLISECONDS = 500;

[[nodiscard]] static auto get_property(xmlNode &node, const char *name) {
  return xml_string_to_string(xmlGetProp(&node, c_string_to_xml_string(name)));
}

struct SongWidget : public QWidget {
  Song song;
  Player player = Player(*this);
  QUndoStack undo_stack = QUndoStack(nullptr);
  QString current_file;
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  SwitchColumn &switch_column = *(new SwitchColumn(undo_stack, song));
  ControlsColumn &controls_column =
      *(new ControlsColumn(song, player.synth, undo_stack, switch_column));
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit SongWidget() {
    row_layout.addWidget(&controls_column, 0, Qt::AlignTop);
    row_layout.addWidget(&switch_column, 0, Qt::AlignTop);
  }

  ~SongWidget() override { undo_stack.disconnect(); }

  // prevent moving and copying
  SongWidget(const SongWidget &) = delete;
  auto operator=(const SongWidget &) -> SongWidget = delete;
  SongWidget(SongWidget &&) = delete;
  auto operator=(SongWidget &&) -> SongWidget = delete;
};

[[nodiscard]] static inline   auto get_next_row(const SongWidget &song_widget) {
  return get_only_range(song_widget.switch_column).bottom() + 1;
}

static void initialize_play(SongWidget &song_widget) {
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  initialize_playstate(song, player.play_state,
                       fluid_sequencer_get_tick(&player.sequencer));

  auto &channel_schedules = player.channel_schedules;
  for (auto index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

template <NoteInterface SubNote>
[[nodiscard]] static auto
play_all_notes(Player &player, const int chord_number,
               const QList<SubNote> &sub_notes) -> bool {
  return play_notes(player, chord_number, sub_notes, 0,
                    static_cast<int>(sub_notes.size()));
}

static void update_final_time(Player &player, const double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
}

static void play_chords(SongWidget &song_widget, const int first_chord_number,
                        const int number_of_chords, const int wait_frames = 0) {
  auto &player = song_widget.player;
  auto &play_state = player.play_state;
  const auto &song = song_widget.song;

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
        play_all_notes(player, chord_number, chord.pitched_notes);
    if (!pitched_result) {
      return;
    }
    const auto unpitched_result =
        play_all_notes(player, chord_number, chord.unpitched_notes);
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

static inline auto get_gain_internal(const SongWidget &song_widget) -> double {
  return fluid_synth_get_gain(&song_widget.player.synth);
}

static inline void export_to_file_internal(SongWidget &song_widget,
                                           const QString &output_file) {
  Q_ASSERT(output_file.isValidUtf16());
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  auto &settings = player.settings;
  auto &event = player.event;
  auto &sequencer = player.sequencer;

  stop_playing(sequencer, event);
  maybe_delete_audio_driver(player.audio_driver_pointer);
  check_fluid_ok(fluid_settings_setstr(&settings, "audio.file.name",
                                       output_file.toStdString().c_str()));

  set_fluid_int(settings, "synth.lock-memory", 0);

  auto finished = false;
  const auto finished_timer_id = fluid_sequencer_register_client(
      &player.sequencer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        get_reference(static_cast<bool *>(data_pointer)) = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(song_widget);
  play_chords(song_widget, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  fluid_event_set_dest(&event, finished_timer_id);
  fluid_event_timer(&event, nullptr);
  send_event_at(sequencer, event, player.final_time + START_END_MILLISECONDS);

  auto &renderer = get_reference(new_fluid_file_renderer(&player.synth));
  while (!finished) {
    check_fluid_ok(fluid_file_renderer_process_block(&renderer));
  }
  delete_fluid_file_renderer(&renderer);

  fluid_event_set_dest(&event, player.sequencer_id);
  set_fluid_int(settings, "synth.lock-memory", 1);
  player.audio_driver_pointer =
      make_audio_driver(player.parent, player.settings, player.synth);
}

static void set_xml_double(xmlNode &node, const char *const field_name,
                           double value) {
  set_xml_string(node, field_name, std::to_string(value));
}

static inline void save_as_file_internal(SongWidget &song_widget,
                                         const QString &filename) {
  Q_ASSERT(filename.isValidUtf16());
  const auto &song = song_widget.song;

  XMLDocument document;
  auto &song_node = make_root(document, "song");

  set_xml_double(song_node, "gain", get_gain_internal(song_widget));
  set_xml_double(song_node, "starting_key", song.starting_key);
  set_xml_double(song_node, "starting_tempo", song.starting_tempo);
  set_xml_double(song_node, "starting_velocity", song.starting_velocity);

  maybe_set_xml_rows(song_node, "chords", song.chords);

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

static inline void open_file_internal(SongWidget &song_widget,
                                      const QString &filename) {

  Q_ASSERT(filename.isValidUtf16());
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &chords_model = song_widget.switch_column.chords_table.model;

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
    } else {
      Q_ASSERT(false);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }

  song_widget.current_file = filename;

  clear_and_clean(undo_stack);
}

[[nodiscard]] static auto node_is(const xmlNode &node, const char *name) {
  return get_xml_name(node) == name;
}

[[nodiscard]] static auto get_xml_child(xmlNode &node,
                                        const char *name) -> auto & {
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
  return std::max_element(notes.begin(), notes.end(),
                          [](const MusicXMLNote &first_note,
                             const MusicXMLNote &second_note) {
                            return first_note.duration < second_note.duration;
                          })
      ->duration;
}

static void add_chord(ChordsModel &chords_model,
                      const MusicXMLChord &parse_chord,
                      const int measure_number, const int key,
                      const int last_midi_key, const int song_divisions,
                      int time_delta) {
  Chord new_chord;
  new_chord.beats = Rational(time_delta, song_divisions);
  new_chord.interval = get_interval(key - last_midi_key);
  new_chord.words = QString::number(measure_number);
  auto &unpitched_notes = new_chord.unpitched_notes;
  for (const auto &parse_unpitched_note : parse_chord.unpitched_notes) {
    UnpitchedNote new_note;
    new_note.beats = Rational(parse_unpitched_note.duration, song_divisions);
    new_note.words = parse_unpitched_note.words;
    unpitched_notes.push_back(std::move(new_note));
  }
  auto &pitched_notes = new_chord.pitched_notes;
  for (const auto &parse_pitched_note : parse_chord.pitched_notes) {
    PitchedNote new_note;
    new_note.beats = Rational(parse_pitched_note.duration, song_divisions);
    new_note.words = parse_pitched_note.words;
    new_note.interval = get_interval(parse_pitched_note.midi_number - key);
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
                                          int time) -> int {
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
          time_per_division *
              (check_divisions_time - iterator.next_change_divisions_time),
      time_per_division);
}

// TODO(brandon): transposing instruments
static inline void import_musicxml_internal(SongWidget &song_widget,
                                            const QString &filename) {
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &chords_model = song_widget.switch_column.chords_table.model;

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

      auto *measure_pointer = xmlFirstElementChild(&part_node);
      while (measure_pointer != nullptr) {
        auto &measure = get_reference(measure_pointer);
        part_measure_number_dict[current_time] = measure_number;
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
                QMessageBox::warning(
                    &song_widget, QObject::tr("Transpose error"),
                    QObject::tr("Transposition not supported"));
                return; // endpoint
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
                instrument_name =
                    part_info.instrument_map[get_property(note_field, "id")];
              }
              note_field_pointer = xmlNextElementSibling(note_field_pointer);
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
          }
          measure_element_pointer =
              xmlNextElementSibling(measure_element_pointer);
        }
        measure_number++;
        measure_pointer = xmlNextElementSibling(&measure);
      }
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

  clear_rows(chords_model);

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
    const QKeySequence::StandardKey key_sequence = QKeySequence::StandardKey(),
    const bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}
