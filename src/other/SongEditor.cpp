#include "justly/SongEditor.hpp"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAction>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaProperty> // IWYU pragma: keep
#include <QMetaType>
#include <QMimeData>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QStyleOption>
#include <QTextStream>
#include <QUndoStack>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "cell_editors/InstrumentEditor.hpp"
#include "cell_values/Instrument.hpp"
#include "cell_values/Interval.hpp"
#include "cell_values/Rational.hpp"
#include "changes/InsertChords.hpp"
#include "changes/InsertNotes.hpp"
#include "changes/SetCells.hpp"
#include "changes/SetGain.hpp"
#include "changes/SetStartingInstrument.hpp"
#include "changes/SetStartingKey.hpp"
#include "changes/SetStartingTempo.hpp"
#include "changes/SetStartingVelocity.hpp"
#include "models/ChordsModel.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "note_chord/NoteChord.hpp"
#include "other/ChordsView.hpp"
#include "other/RowRange.hpp"
#include "other/bounds.hpp"
#include "other/conversions.hpp"
#include "other/templates.hpp"

static const auto NUMBER_OF_MIDI_CHANNELS = 16;

static const auto MAX_GAIN = 10;
static const auto GAIN_STEP = 0.1;

static const auto MIN_STARTING_KEY = 60;
static const auto MAX_STARTING_KEY = 440;

static const auto MIN_STARTING_TEMPO = 25;
static const auto MAX_STARTING_TEMPO = 200;

static const auto SECONDS_PER_MINUTE = 60;
static const unsigned int MILLISECONDS_PER_SECOND = 1000;

static const auto MAX_VELOCITY = 127;

static const auto BEND_PER_HALFSTEP = 4096;
static const auto ZERO_BEND_HALFSTEPS = 2;

// insert end buffer at the end of songs
static const unsigned int START_END_MILLISECONDS = 500;

static const auto VERBOSE_FLUIDSYNTH = false;

static const auto CHORDS_MIME = "application/prs.chords+json";
static const auto NOTES_MIME = "application/prs.notes+json";
static const auto CELLS_MIME = "application/prs.cells+json";

[[nodiscard]] static auto get_settings_pointer() {
  fluid_settings_t *settings_pointer = new_fluid_settings();
  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    auto cores_result = fluid_settings_setint(
        settings_pointer, "synth.cpu-cores", static_cast<int>(cores));
    Q_ASSERT(cores_result == FLUID_OK);
  }
  if (VERBOSE_FLUIDSYNTH) {
    auto verbose_result =
        fluid_settings_setint(settings_pointer, "synth.verbose", 1);
    Q_ASSERT(verbose_result == FLUID_OK);
  }
  return settings_pointer;
}

[[nodiscard]] static auto get_soundfont_id(fluid_synth_t *synth_pointer) {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath("../share/MuseScore_General.sf2")
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto maybe_soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(maybe_soundfont_id != -1);
  return maybe_soundfont_id;
}

[[nodiscard]] static auto rational_to_double(const Rational &rational) {
  Q_ASSERT(rational.denominator != 0);
  return (1.0 * rational.numerator) / rational.denominator;
}

[[nodiscard]] static auto get_json_value(const nlohmann::json &json,
                                         const std::string &field) {
  Q_ASSERT(json.contains(field));
  return json[field];
}

[[nodiscard]] static auto get_json_double(const nlohmann::json &json,
                                          const std::string &field) {
  const auto &json_value = get_json_value(json, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<double>();
}

template <typename Edtior, typename Value>
static auto set_value_no_signals(Edtior *editor_pointer, Value new_value) {
  editor_pointer->blockSignals(true);
  editor_pointer->setValue(new_value);
  editor_pointer->blockSignals(false);
}

[[nodiscard]] static auto copy_json(const nlohmann::json &copied,
                                    const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  new_data_pointer->setData(mime_type, json_text.str().c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  if (mime_type == CHORDS_MIME) {
    return ChordsModel::tr("chords");
  }
  if (mime_type == NOTES_MIME) {
    return ChordsModel::tr("notes");
  }
  if (mime_type == CELLS_MIME) {
    return ChordsModel::tr("cells");
  }
  return mime_type;
}

[[nodiscard]] static auto
parse_clipboard(QWidget *parent_pointer, const QString &mime_type,
                const nlohmann::json_schema::json_validator &validator) {
  const auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  const auto *mime_data_pointer = clipboard_pointer->mimeData();
  Q_ASSERT(mime_data_pointer != nullptr);

  if (!mime_data_pointer->hasFormat(mime_type)) {
    auto formats = mime_data_pointer->formats();
    Q_ASSERT(!(formats.empty()));
    QString message;
    QTextStream stream(&message);
    stream << ChordsModel::tr("Cannot paste ")
           << get_mime_description(formats[0])
           << ChordsModel::tr(" into destination needing ")
           << get_mime_description(mime_type);
    QMessageBox::warning(parent_pointer, ChordsModel::tr("MIME type error"),
                         message);
    return nlohmann::json();
  }
  const auto &copied_text = mime_data_pointer->data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Parsing error"),
                         parse_error.what());
    return nlohmann::json();
  }
  if (copied.empty()) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Empty paste"),
                         "Nothing to paste!");
    return nlohmann::json();
  }
  try {
    validator.validate(copied);
  } catch (const std::exception &error) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Schema error"),
                         error.what());
    return nlohmann::json();
  }
  return copied;
}

[[nodiscard]] static auto
get_note_chord_column_schema(const std::string &description) {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", type_column},
                         {"maximum", words_column}});
}

[[nodiscard]] static auto
get_instrument_schema(const std::string &description) {
  static const std::vector<std::string> instrument_names = []() {
    std::vector<std::string> temp_names;
    const auto &all_instruments = get_all_instruments();
    std::transform(
        all_instruments.cbegin(), all_instruments.cend(),
        std::back_inserter(temp_names),
        [](const Instrument &instrument) { return instrument.name; });
    return temp_names;
  }();
  return nlohmann::json({{"type", "string"},
                         {"description", description},
                         {"enum", instrument_names}});
}

[[nodiscard]] static auto get_rational_schema(const std::string &description) {
  return nlohmann::json({{"type", "object"},
                         {"description", description},
                         {"properties",
                          {{"numerator",
                            {{"type", "integer"},
                             {"description", "the numerator"},
                             {"minimum", 1},
                             {"maximum", MAX_RATIONAL_NUMERATOR}}},
                           {"denominator",
                            {{"type", "integer"},
                             {"description", "the denominator"},
                             {"minimum", 1},
                             {"maximum", MAX_RATIONAL_DENOMINATOR}}}}}});
}

[[nodiscard]] static auto get_note_chord_columns_schema() {
  return nlohmann::json(
      {{"instrument", get_instrument_schema("the instrument")},
       {"interval",
        {{"type", "object"},
         {"description", "an interval"},
         {"properties",
          {{"numerator",
            {{"type", "integer"},
             {"description", "the numerator"},
             {"minimum", 1},
             {"maximum", MAX_INTERVAL_NUMERATOR}}},
           {"denominator",
            {{"type", "integer"},
             {"description", "the denominator"},
             {"minimum", 1},
             {"maximum", MAX_INTERVAL_DENOMINATOR}}},
           {"octave",
            {{"type", "integer"},
             {"description", "the octave"},
             {"minimum", MIN_OCTAVE},
             {"maximum", MAX_OCTAVE}}}}}}},
       {"beats", get_rational_schema("the number of beats")},
       {"velocity_percent", get_rational_schema("velocity ratio")},
       {"tempo_percent", get_rational_schema("tempo ratio")},
       {"words", {{"type", "string"}, {"description", "the words"}}}});
}

[[nodiscard]] static auto get_notes_schema() -> const nlohmann::json & {
  static const nlohmann::json notes_schema(
      {{"type", "array"},
       {"description", "the notes"},
       {"items",
        {{"type", "object"},
         {"description", "a note"},
         {"properties", get_note_chord_columns_schema()}}}});
  return notes_schema;
}

auto static get_chords_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema = []() {
    auto chord_properties = get_note_chord_columns_schema();
    chord_properties["notes"] = get_notes_schema();
    return nlohmann::json({{"type", "array"},
                           {"description", "a list of chords"},
                           {"items",
                            {{"type", "object"},
                             {"description", "a chord"},
                             {"properties", std::move(chord_properties)}}}});
  }();
  return chord_schema;
}

[[nodiscard]] static auto make_validator(const std::string &title,
                                         nlohmann::json json) {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return nlohmann::json_schema::json_validator(json);
}

static auto paste_rows(ChordsModel &chords_model, size_t first_child_number,
                       const QModelIndex &parent_index) {
  auto *undo_stack_pointer = chords_model.undo_stack_pointer;

  auto *parent_pointer = chords_model.parent_pointer;

  if (is_root_index(parent_index)) {
    static const nlohmann::json_schema::json_validator chords_validator =
        make_validator("Chords", get_chords_schema());
    auto json_chords =
        parse_clipboard(parent_pointer, CHORDS_MIME, chords_validator);
    if (json_chords.empty()) {
      return;
    }
    std::vector<Chord> new_chords;
    json_to_chords(new_chords, json_chords);
    undo_stack_pointer->push(
        std::make_unique<InsertChords>(&chords_model, first_child_number,
                                       std::move(new_chords))
            .release());
  } else {
    static const nlohmann::json_schema::json_validator notes_validator =
        make_validator("Notes", get_notes_schema());
    auto json_notes =
        parse_clipboard(parent_pointer, NOTES_MIME, notes_validator);
    if (json_notes.empty()) {
      return;
    }
    std::vector<Note> new_notes;
    json_to_notes(new_notes, json_notes);
    undo_stack_pointer->push(std::make_unique<InsertNotes>(
                                 &chords_model, get_child_number(parent_index),
                                 first_child_number, std::move(new_notes))
                                 .release());
  }
}

static auto note_chords_from_json(std::vector<NoteChord> &new_note_chords,
                                  const nlohmann::json &json_note_chords,
                                  size_t number_to_write) {
  std::transform(json_note_chords.cbegin(),
                 json_note_chords.cbegin() + static_cast<int>(number_to_write),
                 std::back_inserter(new_note_chords),
                 [](const nlohmann::json &json_template) {
                   return NoteChord(json_template);
                 });
}

[[nodiscard]] static auto
get_number_of_rows_left(const std::vector<Chord> &chords,
                        size_t first_chord_number) {
  return std::accumulate(chords.begin() + static_cast<int>(first_chord_number),
                         chords.end(), static_cast<size_t>(0),
                         [](int total, const Chord &chord) {
                           // add 1 for the chord itself
                           return total + 1 + chord.notes.size();
                         });
}

[[nodiscard]] static auto is_rows(const QItemSelection &selection) {
  // selecting a type column selects the whole row
  return selection[0].left() == type_column;
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelection &selection) {
  return std::accumulate(
      selection.cbegin(), selection.cend(), static_cast<size_t>(0),
      [](size_t total, const QItemSelectionRange &next_range) {
        return total + next_range.bottom() - next_range.top() + 1;
      });
}

[[nodiscard]] static auto to_row_ranges(const QItemSelection &selection) {
  std::vector<RowRange> row_ranges;
  for (const auto &range : selection) {
    const auto row_range = RowRange(range);
    auto parent_number = row_range.parent_number;
    if (parent_number == -1) {
      // separate chords because there are notes in between
      auto end_child_number = get_end_child_number(row_range);
      for (auto child_number = row_range.first_child_number;
           child_number < end_child_number; child_number++) {
        row_ranges.emplace_back(child_number, 1, parent_number);
      }
    } else {
      row_ranges.push_back(row_range);
    }
  }
  // sort to collate chords and notes
  std::sort(row_ranges.begin(), row_ranges.end(),
            [](const RowRange &row_range_1, const RowRange &row_range_2) {
              return first_is_less(row_range_1, row_range_2);
            });
  return row_ranges;
}

static auto add_row_ranges_from(const std::vector<Chord> &chords,
                                std::vector<RowRange> &row_ranges,
                                size_t chord_number,
                                size_t number_of_note_chords) {
  while (number_of_note_chords > 0) {
    row_ranges.emplace_back(chord_number, 1, -1);
    number_of_note_chords = number_of_note_chords - 1;
    if (number_of_note_chords == 0) {
      break;
    }
    auto number_of_notes = get_const_item(chords, chord_number).notes.size();
    if (number_of_notes > 0) {
      if (number_of_note_chords <= number_of_notes) {
        row_ranges.emplace_back(0, number_of_note_chords, chord_number);
        break;
      }
      row_ranges.emplace_back(0, number_of_notes, chord_number);
      number_of_note_chords = number_of_note_chords - number_of_notes;
    }
    chord_number = chord_number + 1;
  }
}

[[nodiscard]] static auto
get_note_chords_from_ranges(const std::vector<Chord> &chords,
                            const std::vector<RowRange> &row_ranges) {
  std::vector<NoteChord> note_chords;
  for (const auto &row_range : row_ranges) {
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;

    if (is_chords(row_range)) {
      check_range(chords, first_child_number, number_of_children);
      note_chords.insert(note_chords.end(),
                         chords.cbegin() + static_cast<int>(first_child_number),
                         chords.cbegin() +
                             static_cast<int>(get_end_child_number(row_range)));

    } else {
      const auto &chord = chords[get_parent_chord_number(row_range)];
      const auto &notes = chord.notes;
      check_range(notes, first_child_number, number_of_children);
      note_chords.insert(note_chords.end(),
                         notes.cbegin() + static_cast<int>(first_child_number),
                         notes.cbegin() + static_cast<int>(first_child_number +
                                                           number_of_children));
    }
  }
  return note_chords;
}

static auto add_cell_changes(ChordsModel &chords_model,
                             const std::vector<RowRange> &row_ranges,
                             const std::vector<NoteChord> &new_note_chords,
                             NoteChordColumn left_column,
                             NoteChordColumn right_column) {
  auto *undo_stack_pointer = chords_model.undo_stack_pointer;
  undo_stack_pointer->push(
      std::make_unique<SetCells>(
          &chords_model, row_ranges,
          get_note_chords_from_ranges(chords_model.chords, row_ranges),
          new_note_chords, left_column, right_column)
          .release());
}

static auto copy_selected(const ChordsView &chords_view) {
  const auto *selection_model_pointer = chords_view.selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  const auto *chords_model_pointer = chords_view.chords_model_pointer;

  const auto &selection = selection_model_pointer->selection();
  const auto &chords = chords_model_pointer->chords;

  if (is_rows(selection)) {
    Q_ASSERT(selection.size() == 1);
    const auto &row_range = RowRange(selection[0]);
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
    if (is_chords(row_range)) {
      copy_json(chords_to_json(chords, first_child_number, number_of_children),
                CHORDS_MIME);
    } else {
      copy_json(
          notes_to_json(
              get_const_item(chords, get_parent_chord_number(row_range)).notes,
              first_child_number, number_of_children),
          NOTES_MIME);
    }
  } else {
    Q_ASSERT(!selection.empty());
    const auto &first_range = selection[0];
    const auto row_ranges = to_row_ranges(selection);
    const auto note_chords = get_note_chords_from_ranges(chords, row_ranges);
    nlohmann::json json_note_chords;
    std::transform(note_chords.cbegin(), note_chords.cend(),
                   std::back_inserter(json_note_chords),
                   [](const NoteChord &note_chord) {
                     return note_chord_to_json(&note_chord);
                   });
    copy_json(nlohmann::json(
                  {{"left_column", to_note_chord_column(first_range.left())},
                   {"right_column", to_note_chord_column(first_range.right())},
                   {"note_chords", std::move(json_note_chords)}}),
              CELLS_MIME);
  }
}

static auto delete_selected(const ChordsView &chords_view) {
  auto *selection_model_pointer = chords_view.selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  auto *chords_model_pointer = chords_view.chords_model_pointer;

  const auto &selection = selection_model_pointer->selection();

  if (is_rows(selection)) {
    Q_ASSERT(selection.size() == 1);
    const auto &range = selection[0];
    const auto &row_range = RowRange(selection[0]);
    auto removed = chords_model_pointer->removeRows(
        static_cast<int>(row_range.first_child_number),
        static_cast<int>(row_range.number_of_children), range.parent());
    Q_ASSERT(removed);
  } else {
    Q_ASSERT(!selection.empty());
    const auto &first_range = selection[0];
    add_cell_changes(*chords_model_pointer, to_row_ranges(selection),
                     std::vector<NoteChord>(get_number_of_rows(selection)),
                     to_note_chord_column(first_range.left()),
                     to_note_chord_column(first_range.right()));
  }
}

void register_converters() {
  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) {
    auto numerator = rational.numerator;
    auto denominator = rational.denominator;

    QString result;
    QTextStream stream(&result);
    if (numerator != 1) {
      stream << numerator;
    }
    if (denominator != 1) {
      stream << "/" << denominator;
    }
    return result;
  });
  QMetaType::registerConverter<Interval, QString>([](const Interval &interval) {
    auto numerator = interval.numerator;
    auto denominator = interval.denominator;
    auto octave = interval.octave;

    QString result;
    QTextStream stream(&result);
    if (numerator != 1) {
      stream << numerator;
    }
    if (denominator != 1) {
      stream << "/" << denominator;
    }
    if (octave != 0) {
      stream << "o" << octave;
    }
    return result;
  });
  QMetaType::registerConverter<const Instrument *, QString>(
      [](const Instrument *instrument_pointer) {
        Q_ASSERT(instrument_pointer != nullptr);
        return QString::fromStdString(instrument_pointer->name);
      });
}

static auto ask_discard_changes(QWidget *parent_pointer) -> bool {
  return QMessageBox::question(
             parent_pointer, SongEditor::tr("Unsaved changes"),
             SongEditor::tr("Discard unsaved changes?")) == QMessageBox::Yes;
}

auto SongEditor::get_selected_file(QFileDialog *dialog_pointer) -> QString {
  current_folder = dialog_pointer->directory().absolutePath();
  const auto &selected_files = dialog_pointer->selectedFiles();
  Q_ASSERT(!(selected_files.empty()));
  return selected_files[0];
}

auto SongEditor::beat_time() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void SongEditor::send_event_at(double time) const {
  Q_ASSERT(time >= 0);
  auto result =
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1);
  Q_ASSERT(result == FLUID_OK);
};

void SongEditor::start_real_time() {
  static const std::string default_driver = [this]() -> std::string {
    auto default_driver_pointer = std::make_unique<char *>();
    fluid_settings_dupstr(settings_pointer, "audio.driver",
                          default_driver_pointer.get());
    Q_ASSERT(default_driver_pointer != nullptr);
    return *default_driver_pointer;
  }();

  delete_audio_driver();

#ifdef __linux__
  fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#else
  fluid_settings_setstr(settings_pointer, "audio.driver",
                        default_driver.c_str());
#endif

#ifndef NO_REALTIME_AUDIO
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
#endif
  if (audio_driver_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << tr("Cannot start audio driver \"")
           << QString::fromStdString(default_driver) << tr("\"");
#ifdef NO_WARN_AUDIO
    qWarning("%s", message.toStdString().c_str());
#else
    QMessageBox::warning(this, tr("Audio driver error"), message);
#endif
  }
}

void SongEditor::initialize_play() {
  current_key = get_starting_key();
  current_velocity = get_starting_velocity();
  current_tempo = get_starting_tempo();
  current_instrument_pointer =
      chords_model_pointer->starting_instrument_pointer;

  starting_time = fluid_sequencer_get_tick(sequencer_pointer);
  current_time = starting_time;

  for (size_t index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = current_time;
  }
}

void SongEditor::modulate(const Chord &chord) {
  current_key = current_key * interval_to_double(chord.interval);
  current_velocity =
      current_velocity * rational_to_double(chord.velocity_ratio);
  current_tempo = current_tempo * rational_to_double(chord.tempo_ratio);
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  Q_ASSERT(chord_instrument_pointer != nullptr);
  if (!instrument_is_default(*chord_instrument_pointer)) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

auto SongEditor::play_notes(size_t chord_index, const Chord &chord,
                            size_t first_note_index,
                            size_t number_of_notes) -> double {
  auto final_time = current_time;
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    const auto &note = get_const_item(chord.notes, note_index);

    const auto *note_instrument_pointer = note.instrument_pointer;

    Q_ASSERT(note_instrument_pointer != nullptr);
    const auto &instrument_pointer =
        (instrument_is_default(*note_instrument_pointer)
             ? current_instrument_pointer
             : note_instrument_pointer);

    auto midi_float = get_midi(current_key * interval_to_double(note.interval));
    auto closest_midi = round(midi_float);
    auto int_closest_midi = static_cast<int16_t>(closest_midi);

    auto channel_number = -1;
    for (size_t channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
         channel_index = channel_index + 1) {
      if (current_time >= get_const_item(channel_schedules, channel_index)) {
        channel_number = static_cast<int>(channel_index);
        break;
      }
    }

    if (channel_number == -1) {
      QString message;
      QTextStream stream(&message);
      stream << tr("Out of MIDI channels for chord ") << chord_index + 1
             << tr(", note ") << note_index + 1 << tr(". Not playing note.");
      QMessageBox::warning(this, tr("MIDI channel error"), message);
    } else {
      Q_ASSERT(instrument_pointer != nullptr);
      fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                                 instrument_pointer->bank_number,
                                 instrument_pointer->preset_number);
      send_event_at(current_time);

      fluid_event_pitch_bend(
          event_pointer, channel_number,
          static_cast<int>(
              std::round((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
                         BEND_PER_HALFSTEP)));
      send_event_at(current_time + 1);

      auto new_velocity =
          current_velocity * rational_to_double(note.velocity_ratio);
      if (new_velocity > MAX_VELOCITY) {
        QString message;
        QTextStream stream(&message);
        stream << tr("Velocity exceeds ") << MAX_VELOCITY << tr(" for chord ")
               << chord_index + 1 << tr(", note ") << note_index + 1
               << tr(". Playing with velocity ") << MAX_VELOCITY << tr(".");
        QMessageBox::warning(this, tr("Velocity error"), message);
        new_velocity = 1;
      }

      fluid_event_noteon(event_pointer, channel_number, int_closest_midi,
                         static_cast<int16_t>(std::round(new_velocity)));
      send_event_at(current_time + 2);

      auto end_time =
          current_time + (beat_time() * rational_to_double(note.beats) *
                          rational_to_double(note.tempo_ratio)) *
                             MILLISECONDS_PER_SECOND;

      fluid_event_noteoff(event_pointer, channel_number, int_closest_midi);
      send_event_at(end_time);
      Q_ASSERT(to_size_t(channel_number) < channel_schedules.size());
      channel_schedules[channel_number] = end_time;

      if (end_time > final_time) {
        final_time = end_time;
      }
    }
  }
  return final_time;
}

auto SongEditor::play_chords(size_t first_chord_number, size_t number_of_chords,
                             int wait_frames) -> double {
  current_time = current_time + wait_frames;
  auto final_time = current_time;
  for (auto chord_index = first_chord_number;
       chord_index < first_chord_number + number_of_chords;
       chord_index = chord_index + 1) {
    const auto &chord =
        get_const_item(chords_model_pointer->chords, chord_index);

    modulate(chord);
    auto end_time = play_notes(chord_index, chord, 0, chord.notes.size());
    if (end_time > final_time) {
      final_time = end_time;
    }
    current_time =
        current_time + (beat_time() * rational_to_double(chord.beats)) *
                           MILLISECONDS_PER_SECOND;
  }
  return final_time;
}

void SongEditor::stop_playing() const {
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

void SongEditor::delete_audio_driver() {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
    audio_driver_pointer = nullptr;
  }
}

void SongEditor::update_actions() const {
  auto *selection_model_pointer = chords_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);

  auto anything_selected = selection_model_pointer->hasSelection();

  auto selected_row_indexes = selection_model_pointer->selectedRows();
  auto any_rows_selected = !selected_row_indexes.empty();
  auto chords_selected =
      any_rows_selected && valid_is_chord_index(selected_row_indexes[0]);
  auto can_contain = chords_model_pointer->rowCount(QModelIndex()) == 0 ||
                     (chords_selected && selected_row_indexes.size() == 1);

  cut_action_pointer->setEnabled(anything_selected);
  copy_action_pointer->setEnabled(anything_selected);
  insert_after_action_pointer->setEnabled(any_rows_selected);
  delete_action_pointer->setEnabled(anything_selected);
  play_action_pointer->setEnabled(any_rows_selected);
  paste_cells_or_after_action_pointer->setEnabled(anything_selected);
  insert_into_action_pointer->setEnabled(can_contain);
  paste_into_action_pointer->setEnabled(can_contain);
  expand_action_pointer->setEnabled(chords_selected);
  collapse_action_pointer->setEnabled(chords_selected);
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      gain_editor_pointer(new QDoubleSpinBox(this)),
      starting_instrument_editor_pointer(new InstrumentEditor(this, false)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_velocity_editor_pointer(new QDoubleSpinBox(this)),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      undo_stack_pointer(new QUndoStack(this)),
      chords_view_pointer(new ChordsView(undo_stack_pointer, this)),
      chords_model_pointer(chords_view_pointer->chords_model_pointer),
      insert_after_action_pointer(new QAction(tr("Row &after"), this)),
      insert_into_action_pointer(new QAction(tr("Row &into start"), this)),
      delete_action_pointer(new QAction(tr("&Delete"), this)),
      cut_action_pointer(new QAction(tr("&Cut"), this)),
      copy_action_pointer(new QAction(tr("&Copy"), this)),
      paste_cells_or_after_action_pointer(
          new QAction(tr("&Cells, or Rows &after"), this)),
      paste_into_action_pointer(new QAction(tr("Rows &into start"), this)),
      save_action_pointer(new QAction(tr("&Save"), this)),
      play_action_pointer(new QAction(tr("&Play selection"), this)),
      stop_playing_action_pointer(new QAction(tr("&Stop playing"), this)),
      expand_action_pointer(new QAction(tr("&Expand"), this)),
      collapse_action_pointer(new QAction(tr("&Collapse"), this)),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      current_instrument_pointer(get_instrument_pointer("")),
      channel_schedules(std::vector<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      settings_pointer(get_settings_pointer()),
      event_pointer(new_fluid_event()),
      sequencer_pointer(new_fluid_sequencer2(0)),
      synth_pointer(new_fluid_synth(settings_pointer)),
      soundfont_id(get_soundfont_id(synth_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
  statusBar()->showMessage(tr(""));

  auto *controls_pointer = std::make_unique<QFrame>(this).release();
  controls_pointer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QDockWidget *dock_widget_pointer =
      std::make_unique<QDockWidget>("Controls", this).release();

  auto *menu_bar_pointer = menuBar();
  Q_ASSERT(menu_bar_pointer != nullptr);

  auto *file_menu_pointer =
      std::make_unique<QMenu>(tr("&File"), this).release();

  auto *open_action_pointer =
      std::make_unique<QAction>(tr("&Open"), file_menu_pointer).release();
  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, [this]() {
    if (undo_stack_pointer->isClean() || ask_discard_changes(this)) {
      auto *dialog_pointer = make_file_dialog(
          tr("Open — Justly"), "JSON file (*.json)", QFileDialog::AcceptOpen,
          ".json", QFileDialog::ExistingFile);
      if (dialog_pointer->exec() != 0) {
        open_file(get_selected_file(dialog_pointer));
      }
    }
  });
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this,
          [this]() { save_as_file(current_file); });
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto *save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this, [this]() {
    auto *dialog_pointer = make_file_dialog(
        tr("Save As — Justly"), "JSON file (*.json)", QFileDialog::AcceptSave,
        ".json", QFileDialog::AnyFile);

    if (dialog_pointer->exec() != 0) {
      save_as_file(get_selected_file(dialog_pointer));
    }
  });
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_action_pointer, &QAction::triggered, this, [this]() {
    auto *dialog_pointer =
        make_file_dialog(tr("Export — Justly"), "WAV file (*.wav)",
                         QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
    dialog_pointer->setLabelText(QFileDialog::Accept, "Export");
    if (dialog_pointer->exec() != 0) {
      export_to_file(get_selected_file(dialog_pointer));
    }
  });
  file_menu_pointer->addAction(export_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto *edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();

  auto *undo_action_pointer =
      undo_stack_pointer->createUndoAction(edit_menu_pointer);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto *redo_action_pointer =
      undo_stack_pointer->createRedoAction(edit_menu_pointer);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);

  edit_menu_pointer->addSeparator();

  cut_action_pointer->setEnabled(false);
  cut_action_pointer->setShortcuts(QKeySequence::Cut);
  connect(cut_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() {
            copy_selected(*chords_view_pointer);
            delete_selected(*chords_view_pointer);
          });
  edit_menu_pointer->addAction(cut_action_pointer);

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() { copy_selected(*chords_view_pointer); });
  edit_menu_pointer->addAction(copy_action_pointer);

  auto *paste_menu_pointer =
      std::make_unique<QMenu>(tr("&Paste"), edit_menu_pointer).release();

  paste_cells_or_after_action_pointer->setEnabled(false);
  connect(
      paste_cells_or_after_action_pointer, &QAction::triggered,
      chords_view_pointer, [this]() {
        auto *selection_model_pointer = chords_view_pointer->selectionModel();
        Q_ASSERT(selection_model_pointer != nullptr);

        auto selected_row_indexes = selection_model_pointer->selectedRows();

        const auto &selection = selection_model_pointer->selection();

        Q_ASSERT(!selection.empty());
        if (is_rows(selection)) {
          Q_ASSERT(selection.size() == 1);
          const auto &range = selection[0];
          paste_rows(*chords_model_pointer, range.bottom() + 1, range.parent());
        } else {
          auto first_selected_row_range = get_first_row_range(selection);
          auto number_of_selected_rows = get_number_of_rows(selection);
          auto first_selected_child_number =
              first_selected_row_range.first_child_number;
          auto first_selected_is_chords = is_chords(first_selected_row_range);

          const auto &chords = chords_model_pointer->chords;

          static const nlohmann::json_schema::json_validator cells_validator =
              make_validator(
                  "Cells",
                  nlohmann::json(
                      {{"description", "cells"},
                       {"type", "object"},
                       {"required",
                        {"left_column", "right_column", "note_chords"}},
                       {"properties",
                        {{"left_column",
                          get_note_chord_column_schema("left NoteChordColumn")},
                         {"right_column", get_note_chord_column_schema(
                                              "right NoteChordColumn")},
                         {"note_chords",
                          {{"type", "array"},
                           {"description", "a list of NoteChords"},
                           {"items",
                            {{"type", "object"},
                             {"description", "a NoteChord"},
                             {"properties",
                              get_note_chord_columns_schema()}}}}}}}}));
          auto json_cells =
              parse_clipboard(chords_model_pointer->parent_pointer, CELLS_MIME,
                              cells_validator);
          if (json_cells.empty()) {
            return;
          }
          Q_ASSERT(json_cells.contains("left_column"));
          auto left_field_value = json_cells["left_column"];
          Q_ASSERT(left_field_value.is_number());

          Q_ASSERT(json_cells.contains("right_column"));
          auto right_field_value = json_cells["right_column"];
          Q_ASSERT(right_field_value.is_number());

          Q_ASSERT(json_cells.contains("note_chords"));
          auto &json_note_chords = json_cells["note_chords"];

          size_t number_of_rows_left = 0;
          auto number_of_note_chords = json_note_chords.size();
          if (first_selected_is_chords) {
            number_of_rows_left =
                get_number_of_rows_left(chords, first_selected_child_number);
          } else {
            auto first_selected_chord_number =
                get_parent_chord_number(first_selected_row_range);
            number_of_rows_left =
                get_const_item(chords, first_selected_chord_number)
                    .notes.size() -
                first_selected_child_number +
                get_number_of_rows_left(chords,
                                        first_selected_chord_number + 1);
          }

          std::vector<NoteChord> new_note_chords;
          if (number_of_selected_rows > number_of_note_chords) {
            size_t written = 0;
            while (number_of_selected_rows - written > number_of_note_chords) {
              note_chords_from_json(new_note_chords, json_note_chords,
                                    number_of_note_chords);
              written = written + number_of_note_chords;
            }
            note_chords_from_json(new_note_chords, json_note_chords,
                                  number_of_selected_rows - written);
          } else {
            note_chords_from_json(new_note_chords, json_note_chords,
                                  number_of_note_chords > number_of_rows_left
                                      ? number_of_rows_left
                                      : number_of_note_chords);
          }
          auto number_to_write = new_note_chords.size();
          std::vector<RowRange> row_ranges;
          if (first_selected_is_chords) {
            add_row_ranges_from(chords, row_ranges, first_selected_child_number,
                                number_to_write);
          } else {
            auto chord_number =
                get_parent_chord_number(first_selected_row_range);
            auto number_of_notes_left =
                get_const_item(chords, chord_number).notes.size() -
                first_selected_child_number;
            if (number_to_write <= number_of_notes_left) {
              row_ranges.emplace_back(first_selected_child_number,
                                      number_to_write, chord_number);
            } else {
              row_ranges.emplace_back(first_selected_child_number,
                                      number_of_notes_left, chord_number);
              add_row_ranges_from(chords, row_ranges, chord_number + 1,
                                  number_to_write - number_of_notes_left);
            }
          }
          add_cell_changes(*chords_model_pointer, row_ranges, new_note_chords,
                           to_note_chord_column(left_field_value.get<int>()),
                           to_note_chord_column(right_field_value.get<int>()));
        }
      });
  paste_cells_or_after_action_pointer->setShortcuts(QKeySequence::Paste);
  paste_menu_pointer->addAction(paste_cells_or_after_action_pointer);

  paste_into_action_pointer->setEnabled(false);
  connect(paste_into_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() {
            auto *selection_model_pointer =
                chords_view_pointer->selectionModel();
            Q_ASSERT(selection_model_pointer != nullptr);

            auto selected_row_indexes = selection_model_pointer->selectedRows();

            paste_rows(*chords_model_pointer, to_size_t(0),
                       selected_row_indexes.empty() ? QModelIndex()
                                                    : selected_row_indexes[0]);
          });
  paste_menu_pointer->addAction(paste_into_action_pointer);

  edit_menu_pointer->addMenu(paste_menu_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(insert_after_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() {
            auto *selection_model_pointer =
                chords_view_pointer->selectionModel();
            Q_ASSERT(selection_model_pointer != nullptr);

            auto selected_row_indexes = selection_model_pointer->selectedRows();
            Q_ASSERT(!selected_row_indexes.empty());
            const auto &last_index =
                selected_row_indexes[selected_row_indexes.size() - 1];

            auto inserted = chords_model_pointer->insertRows(
                last_index.row() + 1, 1, last_index.parent());
            Q_ASSERT(inserted);
          });
  insert_menu_pointer->addAction(insert_after_action_pointer);

  insert_into_action_pointer->setEnabled(true);
  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  connect(insert_into_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() {
            auto *selection_model_pointer =
                chords_view_pointer->selectionModel();
            Q_ASSERT(selection_model_pointer != nullptr);
            auto selected_row_indexes = selection_model_pointer->selectedRows();

            auto inserted = chords_model_pointer->insertRows(
                0, 1,
                selected_row_indexes.empty() ? QModelIndex()
                                             : selected_row_indexes[0]);
            Q_ASSERT(inserted);
          });
  insert_menu_pointer->addAction(insert_into_action_pointer);

  delete_action_pointer->setEnabled(false);
  delete_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(delete_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() { delete_selected(*chords_view_pointer); });
  edit_menu_pointer->addAction(delete_action_pointer);

  menu_bar_pointer->addMenu(edit_menu_pointer);

  auto *view_menu_pointer =
      std::make_unique<QMenu>(tr("&View"), this).release();

  auto *view_controls_checkbox_pointer =
      std::make_unique<QAction>(tr("&Controls"), view_menu_pointer).release();

  expand_action_pointer->setEnabled(false);
  connect(expand_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() {
            auto *selection_model_pointer =
                chords_view_pointer->selectionModel();
            Q_ASSERT(selection_model_pointer != nullptr);
            for (const auto &index : selection_model_pointer->selectedRows()) {
              chords_view_pointer->expand(index);
            };
          });
  view_menu_pointer->addAction(expand_action_pointer);

  collapse_action_pointer->setEnabled(false);
  connect(collapse_action_pointer, &QAction::triggered, chords_view_pointer,
          [this]() {
            auto *selection_model_pointer =
                chords_view_pointer->selectionModel();
            Q_ASSERT(selection_model_pointer != nullptr);
            for (const auto &index : selection_model_pointer->selectedRows()) {
              chords_view_pointer->collapse(index);
            };
          });
  view_menu_pointer->addAction(collapse_action_pointer);

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled,
          dock_widget_pointer, &QWidget::setVisible);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  auto *play_menu_pointer =
      std::make_unique<QMenu>(tr("&Play"), this).release();

  play_action_pointer->setEnabled(false);
  play_action_pointer->setShortcuts(QKeySequence::Print);
  connect(play_action_pointer, &QAction::triggered, this, [this]() {
    auto *selection_model_pointer = chords_view_pointer->selectionModel();
    Q_ASSERT(selection_model_pointer != nullptr);
    auto selected_row_indexes = selection_model_pointer->selectedRows();

    Q_ASSERT(!(selected_row_indexes.empty()));
    auto first_index = selected_row_indexes[0];

    auto parent_index = chords_model_pointer->parent(first_index);

    auto first_child_number = get_child_number(first_index);
    auto number_of_children = selected_row_indexes.size();

    stop_playing();
    initialize_play();
    if (is_root_index(parent_index)) {
      if (first_child_number > 0) {
        for (size_t chord_index = 0; chord_index < first_child_number;
             chord_index = chord_index + 1) {
          modulate(get_const_item(chords_model_pointer->chords, chord_index));
        }
      }
      play_chords(first_child_number, number_of_children);
    } else {
      auto chord_number = get_child_number(parent_index);
      for (size_t chord_index = 0; chord_index <= chord_number;
           chord_index = chord_index + 1) {
        modulate(get_const_item(chords_model_pointer->chords, chord_index));
      }
      play_notes(chord_number,
                 get_const_item(chords_model_pointer->chords, chord_number),
                 first_child_number, number_of_children);
    }
  });
  play_menu_pointer->addAction(play_action_pointer);

  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          &SongEditor::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto *controls_form_pointer =
      std::make_unique<QFormLayout>(controls_pointer).release();

  gain_editor_pointer->setMinimum(0);
  gain_editor_pointer->setMaximum(MAX_GAIN);
  gain_editor_pointer->setSingleStep(GAIN_STEP);
  connect(gain_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            undo_stack_pointer->push(
                std::make_unique<SetGain>(this, chords_model_pointer->gain,
                                          new_value)
                    .release());
          });
  set_gain_directly(chords_model_pointer->gain);
  controls_form_pointer->addRow(tr("&Gain:"), gain_editor_pointer);

  starting_instrument_editor_pointer->setValue(
      chords_model_pointer->starting_instrument_pointer);
  connect(starting_instrument_editor_pointer, &QComboBox::currentIndexChanged,
          this, [this](int new_index) {
            undo_stack_pointer->push(
                std::make_unique<SetStartingInstrument>(
                    this, chords_model_pointer->starting_instrument_pointer,
                    &get_all_instruments()[new_index])
                    .release());
          });
  controls_form_pointer->addRow(tr("Starting &instrument:"),
                                starting_instrument_editor_pointer);

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");

  starting_key_editor_pointer->setValue(get_starting_key());

  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            undo_stack_pointer->push(std::make_unique<SetStartingKey>(
                                         this, get_starting_key(), new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_velocity_editor_pointer->setMinimum(0);
  starting_velocity_editor_pointer->setMaximum(MAX_VELOCITY);
  starting_velocity_editor_pointer->setDecimals(1);
  starting_velocity_editor_pointer->setValue(get_starting_velocity());
  connect(starting_velocity_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            undo_stack_pointer->push(
                std::make_unique<SetStartingVelocity>(
                    this, get_starting_velocity(), new_value)
                    .release());
          });
  controls_form_pointer->addRow(tr("Starting &velocity:"),
                                starting_velocity_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setValue(get_starting_tempo());
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);

  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            undo_stack_pointer->push(std::make_unique<SetStartingTempo>(
                                         this, get_starting_tempo(), new_value)
                                         .release());
          });
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  connect(chords_view_pointer->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &SongEditor::update_actions);

  setWindowTitle("Justly");
  setCentralWidget(chords_view_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this, [this]() {
    save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                    !current_file.isEmpty());
  });

  connect(chords_model_pointer, &QAbstractItemModel::rowsInserted, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::rowsRemoved, this,
          &SongEditor::update_actions);
  connect(chords_model_pointer, &QAbstractItemModel::modelReset, this,
          &SongEditor::update_actions);

  const auto *primary_screen_pointer = QGuiApplication::primaryScreen();
  Q_ASSERT(primary_screen_pointer != nullptr);
  resize(sizeHint().width(),
         primary_screen_pointer->availableGeometry().height());

  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
}

SongEditor::~SongEditor() {
  undo_stack_pointer->disconnect();

  delete_audio_driver();
  delete_fluid_event(event_pointer);
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);
}

void SongEditor::closeEvent(QCloseEvent *close_event_pointer) {
  if (!undo_stack_pointer->isClean() && !ask_discard_changes(this)) {
    close_event_pointer->ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
}

auto SongEditor::get_chords_view_pointer() const -> QTreeView * {
  return chords_view_pointer;
}

auto SongEditor::get_gain() const -> double {
  return fluid_synth_get_gain(synth_pointer);
};

auto SongEditor::get_starting_instrument_name() const -> std::string {
  return chords_model_pointer->starting_instrument_pointer->name;
};

auto SongEditor::get_starting_key() const -> double {
  return chords_model_pointer->starting_key;
};

auto SongEditor::get_starting_velocity() const -> double {
  return chords_model_pointer->starting_velocity;
};

auto SongEditor::get_starting_tempo() const -> double {
  return chords_model_pointer->starting_tempo;
};

auto SongEditor::get_current_file() const -> QString { return current_file; };

auto SongEditor::get_chord_index(size_t chord_number,
                                 NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return chords_model_pointer->get_chord_index(chord_number, note_chord_column);
}

auto SongEditor::get_note_index(size_t chord_number, size_t note_number,
                                NoteChordColumn note_chord_column) const
    -> QModelIndex {
  return chords_model_pointer->get_note_index(chord_number, note_number,
                                              note_chord_column);
}

void SongEditor::set_gain(double new_value) const {
  gain_editor_pointer->setValue(new_value);
};

void SongEditor::set_starting_instrument_name(
    const std::string &new_name) const {
  starting_instrument_editor_pointer->setValue(
      get_instrument_pointer(new_name));
}

void SongEditor::set_starting_key(double new_value) const {
  starting_key_editor_pointer->setValue(new_value);
}

void SongEditor::set_starting_velocity(double new_value) const {
  starting_velocity_editor_pointer->setValue(new_value);
}

void SongEditor::set_starting_tempo(double new_value) const {
  starting_tempo_editor_pointer->setValue(new_value);
}

void SongEditor::set_gain_directly(double new_gain) {
  set_value_no_signals(gain_editor_pointer, new_gain);
  chords_model_pointer->gain = new_gain;
  fluid_synth_set_gain(synth_pointer, static_cast<float>(new_gain));
}

void SongEditor::set_starting_instrument_directly(const Instrument *new_value) {
  set_value_no_signals(starting_instrument_editor_pointer, new_value);
  chords_model_pointer->starting_instrument_pointer = new_value;
}

void SongEditor::set_starting_key_directly(double new_value) {
  set_value_no_signals(starting_key_editor_pointer, new_value);
  chords_model_pointer->starting_key = new_value;
}

void SongEditor::set_starting_velocity_directly(double new_value) {
  set_value_no_signals(starting_velocity_editor_pointer, new_value);
  chords_model_pointer->starting_velocity = new_value;
}

auto SongEditor::create_editor(QModelIndex index) const -> QWidget * {
  auto *delegate_pointer = chords_view_pointer->itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  auto *viewport_pointer = chords_view_pointer->viewport();
  Q_ASSERT(viewport_pointer != nullptr);

  auto *cell_editor_pointer = delegate_pointer->createEditor(
      viewport_pointer, QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor_pointer != nullptr);

  delegate_pointer->setEditorData(cell_editor_pointer, index);

  return cell_editor_pointer;
}

void SongEditor::set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                            const QVariant &new_value) const {
  Q_ASSERT(cell_editor_pointer != nullptr);
  const auto *cell_editor_meta_object = cell_editor_pointer->metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor_pointer->setProperty(
      cell_editor_meta_object->userProperty().name(), new_value);

  auto *delegate_pointer = chords_view_pointer->itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  delegate_pointer->setModelData(cell_editor_pointer, chords_model_pointer,
                                 index);
}

void SongEditor::undo() const { undo_stack_pointer->undo(); };
void SongEditor::redo() const { undo_stack_pointer->redo(); };

void SongEditor::trigger_insert_after() const {
  insert_after_action_pointer->trigger();
};
void SongEditor::trigger_insert_into() const {
  insert_into_action_pointer->trigger();
};
void SongEditor::trigger_delete() const { delete_action_pointer->trigger(); };

void SongEditor::trigger_cut() const { cut_action_pointer->trigger(); };
void SongEditor::trigger_copy() const { copy_action_pointer->trigger(); };
void SongEditor::trigger_paste_cells_or_after() const {
  paste_cells_or_after_action_pointer->trigger();
};
void SongEditor::trigger_paste_into() const {
  paste_into_action_pointer->trigger();
};

void SongEditor::trigger_save() const { save_action_pointer->trigger(); };

void SongEditor::trigger_play() const { play_action_pointer->trigger(); };

void SongEditor::trigger_stop_playing() const {
  stop_playing_action_pointer->trigger();
};

void SongEditor::trigger_expand() const { expand_action_pointer->trigger(); };

void SongEditor::trigger_collapse() const {
  collapse_action_pointer->trigger();
};

auto SongEditor::make_file_dialog(
    const QString &caption, const QString &filter,
    QFileDialog::AcceptMode accept_mode, const QString &suffix,
    QFileDialog::FileMode file_mode) -> QFileDialog * {
  auto *dialog_pointer =
      std::make_unique<QFileDialog>(this, caption, current_folder, filter)
          .release();

  dialog_pointer->setAcceptMode(accept_mode);
  dialog_pointer->setDefaultSuffix(suffix);
  dialog_pointer->setFileMode(file_mode);

  return dialog_pointer;
}

void SongEditor::set_starting_tempo_directly(double new_value) {
  set_value_no_signals(starting_tempo_editor_pointer, new_value);
  chords_model_pointer->starting_tempo = new_value;
}

void SongEditor::open_file(const QString &filename) {
  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(this, tr("Parsing error"), parse_error.what());
    return;
  }
  file_io.close();

  static const nlohmann::json_schema::json_validator song_validator =
      make_validator(
          "Song",
          nlohmann::json(
              {{"description", "A Justly song in JSON format"},
               {"type", "object"},
               {"required",
                {"starting_key", "starting_tempo", "starting_velocity",
                 "starting_instrument"}},
               {"properties",
                {{"starting_instrument",
                  get_instrument_schema("the starting instrument")},
                 {"gain",
                  {{"type", "number"},
                   {"description", "the gain (speaker volume)"},
                   {"minimum", 0},
                   {"maximum", MAX_GAIN}}},
                 {"starting_key",
                  {{"type", "number"},
                   {"description", "the starting key, in Hz"},
                   {"minimum", MIN_STARTING_KEY},
                   {"maximum", MAX_STARTING_KEY}}},
                 {"starting_tempo",
                  {{"type", "number"},
                   {"description", "the starting tempo, in bpm"},
                   {"minimum", MIN_STARTING_TEMPO},
                   {"maximum", MAX_STARTING_TEMPO}}},
                 {"starting_velocity",
                  {{"type", "number"},
                   {"description", "the starting velocity (note force)"},
                   {"minimum", 0},
                   {"maximum", MAX_VELOCITY}}},
                 {"chords", get_chords_schema()}}}}));
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    QMessageBox::warning(this, tr("Schema error"), error.what());
    return;
  }

  if (json_song.contains("gain")) {
    set_gain(get_json_double(json_song, "gain"));
  }

  if (json_song.contains("starting_key")) {
    set_starting_key(get_json_double(json_song, "starting_key"));
  }

  if (json_song.contains("starting_velocity")) {
    set_starting_velocity(get_json_double(json_song, "starting_velocity"));
  }

  if (json_song.contains("starting_tempo")) {
    set_starting_tempo(get_json_double(json_song, "starting_tempo"));
  }

  if (json_song.contains("starting_instrument")) {
    const auto &starting_instrument_value =
        get_json_value(json_song, "starting_instrument");
    Q_ASSERT(starting_instrument_value.is_string());
    set_starting_instrument_name(starting_instrument_value.get<std::string>());
  }

  const auto &chords = chords_model_pointer->chords;
  if (!chords.empty()) {
    chords_model_pointer->remove_chords(0, chords.size());
  }

  if (json_song.contains("chords")) {
    chords_model_pointer->append_json_chords(json_song["chords"]);
  }

  current_file = filename;

  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();
}

void SongEditor::save_as_file(const QString &filename) {
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = chords_model_pointer->gain;
  json_song["starting_key"] = get_starting_key();
  json_song["starting_tempo"] = get_starting_tempo();
  json_song["starting_velocity"] = get_starting_velocity();
  json_song["starting_instrument"] = get_starting_instrument_name();

  const auto &chords = chords_model_pointer->chords;
  if (!chords.empty()) {
    json_song["chords"] = chords_to_json(chords, 0, chords.size());
  }

  file_io << std::setw(4) << json_song;
  file_io.close();
  current_file = filename;

  undo_stack_pointer->setClean();
}

void SongEditor::export_to_file(const QString &output_file) {
  stop_playing();

  delete_audio_driver();
  auto file_result = fluid_settings_setstr(settings_pointer, "audio.file.name",
                                           output_file.toStdString().c_str());
  Q_ASSERT(file_result == FLUID_OK);

  auto unlock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);
  Q_ASSERT(unlock_result == FLUID_OK);

  auto finished = false;
  auto finished_timer_id = fluid_sequencer_register_client(
      sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play();
  auto final_time = play_chords(0, chords_model_pointer->chords.size(),
                                START_END_MILLISECONDS);

  fluid_event_set_dest(event_pointer, finished_timer_id);
  fluid_event_timer(event_pointer, nullptr);
  send_event_at(final_time + START_END_MILLISECONDS);

  auto *renderer_pointer = new_fluid_file_renderer(synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    auto process_result = fluid_file_renderer_process_block(renderer_pointer);
    Q_ASSERT(process_result == FLUID_OK);
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(event_pointer, sequencer_id);
  auto lock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  Q_ASSERT(lock_result == FLUID_OK);
  start_real_time();
}
