// TODO: add percussion tests
// TODO: disable percussion for chords
// TODO: add default percussion selector
// TODO: insert chord/insert note
// TODO: refactor ChordsModel functions?

#include "other/SongEditor.hpp"

#include <QAbstractItemModel>
#include <QAction>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
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
#include <QMetaProperty> // IWYU pragma: keep
#include <QMimeData>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
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
#include <variant>
#include <vector>

#include "commands/InsertChords.hpp"
#include "commands/InsertNotes.hpp"
#include "commands/RemoveChords.hpp"
#include "commands/RemoveNotes.hpp"
#include "commands/SetCells.hpp"
#include "commands/SetGain.hpp"
#include "commands/SetStartingKey.hpp"
#include "commands/SetStartingTempo.hpp"
#include "commands/SetStartingVelocity.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteChordColumn.hpp"
#include "justly/justly.hpp"
#include "note_chord/Chord.hpp"
#include "note_chord/Note.hpp"
#include "note_chord/NoteChord.hpp"
#include "other/ChordsModel.hpp"
#include "other/ChordsView.hpp"
#include "other/RowRange.hpp"
#include "other/bounds.hpp"
#include "other/conversions.hpp"
#include "other/templates.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

static const auto NUMBER_OF_MIDI_CHANNELS = 16;

static const auto GAIN_STEP = 0.1;

static const auto SECONDS_PER_MINUTE = 60;
static const unsigned int MILLISECONDS_PER_SECOND = 1000;

static const auto BEND_PER_HALFSTEP = 4096;
static const auto ZERO_BEND_HALFSTEPS = 2;

static const auto VERBOSE_FLUIDSYNTH = false;

static const auto CHORDS_MIME = "application/prs.chords+json";
static const auto NOTES_MIME = "application/prs.notes+json";
static const auto CELLS_MIME = "application/prs.cells+json";

[[nodiscard]] auto get_beat_time(double tempo) -> double {
  return SECONDS_PER_MINUTE / tempo;
}

template <typename Editor, typename Value>
static auto set_value_no_signals(Editor *editor_pointer, Value new_value) {
  editor_pointer->blockSignals(true);
  editor_pointer->setValue(new_value);
  editor_pointer->blockSignals(false);
}

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

[[nodiscard]] static auto rational_to_double(const Rational &rational) {
  Q_ASSERT(rational.denominator != 0);
  return (1.0 * rational.numerator) / rational.denominator;
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

[[nodiscard]] static auto get_chord_columns_schema() {
  static const std::vector<std::string> percussion_names = []() {
    std::vector<std::string> temp_names;
    const auto &all_percussions = get_all_percussions();
    std::transform(
        all_percussions.cbegin(), all_percussions.cend(),
        std::back_inserter(temp_names),
        [](const Percussion &percussion) { return percussion.name; });
    return temp_names;
  }();
  return nlohmann::json(
      {{"interval",
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

[[nodiscard]] static auto get_note_chord_columns_schema() {
  static const std::vector<std::string> percussion_names = []() {
    std::vector<std::string> temp_names;
    const auto &all_percussions = get_all_percussions();
    std::transform(
        all_percussions.cbegin(), all_percussions.cend(),
        std::back_inserter(temp_names),
        [](const Percussion &percussion) { return percussion.name; });
    return temp_names;
  }();
  auto note_properties = get_chord_columns_schema();
  note_properties["instrument"] = get_instrument_schema("the instrument");
  note_properties["percussion"] =
      nlohmann::json({{"type", "string"},
                      {"description", "the percussion"},
                      {"enum", percussion_names}});
  return note_properties;
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

static auto insert_rows(QUndoStack &undo_stack, ChordsModel &chords_model,
                        size_t first_child_number, size_t number_of_children,
                        const QModelIndex &parent_index) {
  const auto &chords = chords_model.chords;

  if (is_root_index(parent_index)) {
    Chord template_chord;
    if (first_child_number > 0) {
      template_chord.beats =
          get_const_item(chords, first_child_number - 1).beats;
    }

    std::vector<Chord> new_chords;
    for (size_t index = 0; index < number_of_children; index = index + 1) {
      new_chords.push_back(template_chord);
    }
    undo_stack.push(std::make_unique<InsertChords>(&chords_model,
                                                   first_child_number,
                                                   std::move(new_chords))
                        .release());
  } else {
    auto chord_number = get_child_number(parent_index);
    const auto &parent_chord = get_const_item(chords, chord_number);

    Note template_note;

    if (first_child_number == 0) {
      template_note.beats = parent_chord.beats;
      template_note.words = parent_chord.words;
    } else {
      const auto &previous_note =
          get_const_item(parent_chord.notes, first_child_number - 1);

      template_note.beats = previous_note.beats;
      template_note.velocity_ratio = previous_note.velocity_ratio;
      template_note.tempo_ratio = previous_note.tempo_ratio;
      template_note.words = previous_note.words;
      template_note.instrument_pointer = previous_note.instrument_pointer;
    }

    std::vector<Note> new_notes;
    for (size_t index = 0; index < number_of_children; index = index + 1) {
      new_notes.push_back(template_note);
    }
    undo_stack.push(std::make_unique<InsertNotes>(&chords_model, chord_number,
                                                  first_child_number,
                                                  std::move(new_notes))
                        .release());
  }
}

static auto delete_selected(QUndoStack &undo_stack,
                            const ChordsView &chords_view) {
  auto *selection_model_pointer = chords_view.selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  auto *chords_model_pointer = chords_view.chords_model_pointer;

  const auto &selection = selection_model_pointer->selection();

  if (is_rows(selection)) {
    Q_ASSERT(selection.size() == 1);
    const auto &range = selection[0];
    const auto &row_range = RowRange(selection[0]);
    const auto &chords = chords_model_pointer->chords;
    auto first_child_number = row_range.first_child_number;
    auto number_of_children = row_range.number_of_children;
    auto parent_index = range.parent();

    if (is_root_index(parent_index)) {
      check_range(chords, first_child_number, number_of_children);
      undo_stack.push(
          std::make_unique<RemoveChords>(
              chords_model_pointer, first_child_number,
              std::vector<Chord>(
                  chords.cbegin() + static_cast<int>(first_child_number),
                  chords.cbegin() + static_cast<int>(first_child_number +
                                                     number_of_children)))
              .release());
    } else {
      auto chord_number = get_child_number(parent_index);
      const auto &chord = get_const_item(chords, chord_number);
      const auto &notes = chord.notes;
      check_range(notes, first_child_number, number_of_children);
      undo_stack.push(
          std::make_unique<RemoveNotes>(
              chords_model_pointer, chord_number, first_child_number,
              std::vector<Note>(
                  notes.cbegin() + static_cast<int>(first_child_number),
                  notes.cbegin() + static_cast<int>(first_child_number +
                                                    number_of_children)))
              .release());
    }
  } else {
    Q_ASSERT(!selection.empty());
    const auto &first_range = selection[0];
    add_cell_changes(*chords_model_pointer, to_row_ranges(selection),
                     std::vector<NoteChord>(get_number_of_rows(selection)),
                     to_note_chord_column(first_range.left()),
                     to_note_chord_column(first_range.right()));
  }
}

static auto make_file_dialog(SongEditor *song_editor_pointer,
                             const QString &caption, const QString &filter,
                             QFileDialog::AcceptMode accept_mode,
                             const QString &suffix,
                             QFileDialog::FileMode file_mode) {
  Q_ASSERT(song_editor_pointer != nullptr);
  auto *dialog_pointer =
      std::make_unique<QFileDialog>(song_editor_pointer, caption,
                                    song_editor_pointer->current_folder, filter)
          .release();

  dialog_pointer->setAcceptMode(accept_mode);
  dialog_pointer->setDefaultSuffix(suffix);
  dialog_pointer->setFileMode(file_mode);

  return dialog_pointer;
}

static auto ask_discard_changes(QWidget *parent_pointer) -> bool {
  return QMessageBox::question(
             parent_pointer, SongEditor::tr("Unsaved changes"),
             SongEditor::tr("Discard unsaved changes?")) == QMessageBox::Yes;
}

static auto get_selected_file(SongEditor *song_editor_pointer,
                              QFileDialog *dialog_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->current_folder =
      dialog_pointer->directory().absolutePath();
  const auto &selected_files = dialog_pointer->selectedFiles();
  Q_ASSERT(!(selected_files.empty()));
  return selected_files[0];
}

static auto modulate(SongEditor *song_editor_pointer, const Chord &chord) {
  Q_ASSERT(song_editor_pointer != nullptr);
  const auto &interval_or_percussion_pointer =
      chord.interval_or_percussion_pointer;
  Q_ASSERT(std::holds_alternative<Interval>(interval_or_percussion_pointer));
  song_editor_pointer->current_key =
      song_editor_pointer->current_key *
      interval_to_double(std::get<Interval>(interval_or_percussion_pointer));
  song_editor_pointer->current_velocity =
      song_editor_pointer->current_velocity *
      rational_to_double(chord.velocity_ratio);
  song_editor_pointer->current_tempo = song_editor_pointer->current_tempo *
                                       rational_to_double(chord.tempo_ratio);
}

static auto update_final_time(SongEditor *song_editor_pointer,
                              double new_final_time) {
  Q_ASSERT(song_editor_pointer != nullptr);
  if (new_final_time > song_editor_pointer->final_time) {
    song_editor_pointer->final_time = new_final_time;
  }
};

static auto play_notes(SongEditor *song_editor_pointer, size_t chord_index,
                       const Chord &chord, size_t first_note_index,
                       size_t number_of_notes) {
  Q_ASSERT(song_editor_pointer != nullptr);

  auto current_time = song_editor_pointer->current_time;
  auto current_key = song_editor_pointer->current_key;
  auto current_velocity = song_editor_pointer->current_velocity;
  auto current_tempo = song_editor_pointer->current_tempo;

  auto &channel_schedules = song_editor_pointer->channel_schedules;
  auto *sequencer_pointer = song_editor_pointer->sequencer_pointer;
  auto *event_pointer = song_editor_pointer->event_pointer;
  auto soundfont_id = song_editor_pointer->soundfont_id;

  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    const auto &note = get_const_item(chord.notes, note_index);

    const auto *instrument_pointer = note.instrument_pointer;
    Q_ASSERT(instrument_pointer != nullptr);

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
      stream << SongEditor::tr("Out of MIDI channels for chord ")
             << chord_index + 1 << SongEditor::tr(", note ") << note_index + 1
             << SongEditor::tr(". Not playing note.");
      QMessageBox::warning(song_editor_pointer,
                           SongEditor::tr("MIDI channel error"), message);
    } else {
      Q_ASSERT(instrument_pointer != nullptr);
      fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                                 instrument_pointer->bank_number,
                                 instrument_pointer->preset_number);
      send_event_at(sequencer_pointer, event_pointer, current_time);

      int16_t midi_number = -1;

      const auto &interval_or_percussion_pointer =
          note.interval_or_percussion_pointer;
      if (std::holds_alternative<const Percussion *>(
              interval_or_percussion_pointer)) {
        const auto *percussion_pointer =
            std::get<const Percussion *>(interval_or_percussion_pointer);
        Q_ASSERT(percussion_pointer != nullptr);
        midi_number = percussion_pointer->midi_number;
      } else {
        auto midi_float =
            get_midi(current_key * interval_to_double(std::get<Interval>(
                                       interval_or_percussion_pointer)));
        auto closest_midi = round(midi_float);
        midi_number = static_cast<int16_t>(closest_midi);

        fluid_event_pitch_bend(
            event_pointer, channel_number,
            static_cast<int>(
                std::round((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
                           BEND_PER_HALFSTEP)));
        send_event_at(sequencer_pointer, event_pointer, current_time + 1);
      }

      auto new_velocity =
          current_velocity * rational_to_double(note.velocity_ratio);
      if (new_velocity > MAX_VELOCITY) {
        QString message;
        QTextStream stream(&message);
        stream << SongEditor::tr("Velocity exceeds ") << MAX_VELOCITY
               << SongEditor::tr(" for chord ") << chord_index + 1
               << SongEditor::tr(", note ") << note_index + 1
               << SongEditor::tr(". Playing with velocity ") << MAX_VELOCITY
               << SongEditor::tr(".");
        QMessageBox::warning(song_editor_pointer,
                             SongEditor::tr("Velocity error"), message);
        new_velocity = 1;
      }

      fluid_event_noteon(event_pointer, channel_number, midi_number,
                         static_cast<int16_t>(std::round(new_velocity)));
      send_event_at(sequencer_pointer, event_pointer, current_time + 2);

      auto note_end_time =
          current_time +
          get_beat_time(current_tempo * rational_to_double(note.tempo_ratio)) *
              rational_to_double(note.beats) * MILLISECONDS_PER_SECOND;

      fluid_event_noteoff(event_pointer, channel_number, midi_number);
      send_event_at(sequencer_pointer, event_pointer, note_end_time);
      Q_ASSERT(to_size_t(channel_number) < channel_schedules.size());
      channel_schedules[channel_number] = note_end_time;

      update_final_time(song_editor_pointer, note_end_time);
    }
  }
}

static auto update_actions(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  auto *selection_model_pointer =
      song_editor_pointer->chords_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);

  auto anything_selected = selection_model_pointer->hasSelection();

  auto selected_row_indexes = selection_model_pointer->selectedRows();
  auto any_rows_selected = !selected_row_indexes.empty();
  auto chords_selected =
      any_rows_selected && valid_is_chord_index(selected_row_indexes[0]);
  auto can_contain =
      song_editor_pointer->chords_model_pointer->rowCount(QModelIndex()) == 0 ||
      (chords_selected && selected_row_indexes.size() == 1);

  song_editor_pointer->cut_action_pointer->setEnabled(anything_selected);
  song_editor_pointer->copy_action_pointer->setEnabled(anything_selected);
  song_editor_pointer->insert_after_action_pointer->setEnabled(
      any_rows_selected);
  song_editor_pointer->delete_action_pointer->setEnabled(anything_selected);
  song_editor_pointer->play_action_pointer->setEnabled(any_rows_selected);
  song_editor_pointer->paste_cells_or_after_action_pointer->setEnabled(
      anything_selected);
  song_editor_pointer->insert_into_action_pointer->setEnabled(can_contain);
  song_editor_pointer->paste_into_action_pointer->setEnabled(can_contain);
  song_editor_pointer->expand_action_pointer->setEnabled(chords_selected);
  song_editor_pointer->collapse_action_pointer->setEnabled(chords_selected);
}

[[nodiscard]] auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return nlohmann::json_schema::json_validator(json);
}

[[nodiscard]] auto
get_instrument_schema(const std::string &description) -> nlohmann::json {
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

auto get_chords_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema = []() {
    auto chord_properties = get_chord_columns_schema();
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

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      gain_editor_pointer(new QDoubleSpinBox(this)),
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
          this, tr("Open — Justly"), "JSON file (*.json)",
          QFileDialog::AcceptOpen, ".json", QFileDialog::ExistingFile);
      if (dialog_pointer->exec() != 0) {
        open_file(this, get_selected_file(this, dialog_pointer));
      }
    }
  });
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this,
          [this]() { save_as_file(this, current_file); });
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto *save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this, [this]() {
    auto *dialog_pointer = make_file_dialog(
        this, tr("Save As — Justly"), "JSON file (*.json)",
        QFileDialog::AcceptSave, ".json", QFileDialog::AnyFile);

    if (dialog_pointer->exec() != 0) {
      save_as_file(this, get_selected_file(this, dialog_pointer));
    }
  });
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_action_pointer, &QAction::triggered, this, [this]() {
    auto *dialog_pointer =
        make_file_dialog(this, tr("Export — Justly"), "WAV file (*.wav)",
                         QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
    dialog_pointer->setLabelText(QFileDialog::Accept, "Export");
    if (dialog_pointer->exec() != 0) {
      export_to_file(this, get_selected_file(this, dialog_pointer));
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
            delete_selected(*undo_stack_pointer, *chords_view_pointer);
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

            insert_rows(*undo_stack_pointer, *chords_model_pointer,
                        last_index.row() + 1, 1, last_index.parent());
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

            insert_rows(*undo_stack_pointer, *chords_model_pointer, 0, 1,
                        selected_row_indexes.empty() ? QModelIndex()
                                                     : selected_row_indexes[0]);
          });
  insert_menu_pointer->addAction(insert_into_action_pointer);

  delete_action_pointer->setEnabled(false);
  delete_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(
      delete_action_pointer, &QAction::triggered, chords_view_pointer,
      [this]() { delete_selected(*undo_stack_pointer, *chords_view_pointer); });
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

    stop_playing(sequencer_pointer, event_pointer);
    initialize_play(this);
    if (is_root_index(parent_index)) {
      if (first_child_number > 0) {
        for (size_t chord_index = 0; chord_index < first_child_number;
             chord_index = chord_index + 1) {
          modulate(this,
                   get_const_item(chords_model_pointer->chords, chord_index));
        }
      }
      play_chords(this, first_child_number, number_of_children);
    } else {
      auto chord_number = get_child_number(parent_index);
      for (size_t chord_index = 0; chord_index <= chord_number;
           chord_index = chord_index + 1) {
        modulate(this,
                 get_const_item(chords_model_pointer->chords, chord_index));
      }
      play_notes(this, chord_number,
                 get_const_item(chords_model_pointer->chords, chord_number),
                 first_child_number, number_of_children);
    }
  });
  play_menu_pointer->addAction(play_action_pointer);

  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          [this]() { stop_playing(sequencer_pointer, event_pointer); });
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
  set_gain_directly(this, chords_model_pointer->gain);
  controls_form_pointer->addRow(tr("&Gain:"), gain_editor_pointer);

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");

  starting_key_editor_pointer->setValue(get_starting_key(this));

  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            undo_stack_pointer->push(
                std::make_unique<SetStartingKey>(this, get_starting_key(this),
                                                 new_value)
                    .release());
          });
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_velocity_editor_pointer->setMinimum(0);
  starting_velocity_editor_pointer->setMaximum(MAX_VELOCITY);
  starting_velocity_editor_pointer->setDecimals(1);
  starting_velocity_editor_pointer->setValue(get_starting_velocity(this));
  connect(starting_velocity_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            undo_stack_pointer->push(
                std::make_unique<SetStartingVelocity>(
                    this, get_starting_velocity(this), new_value)
                    .release());
          });
  controls_form_pointer->addRow(tr("Starting &velocity:"),
                                starting_velocity_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setValue(get_starting_tempo(this));
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);

  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          [this](double new_value) {
            undo_stack_pointer->push(
                std::make_unique<SetStartingTempo>(
                    this, get_starting_tempo(this), new_value)
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
          [this]() { update_actions(this); });

  setWindowTitle("Justly");
  setCentralWidget(chords_view_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this, [this]() {
    save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                    !current_file.isEmpty());
  });

  connect(chords_model_pointer, &QAbstractItemModel::rowsInserted, this,
          [this]() { update_actions(this); });
  connect(chords_model_pointer, &QAbstractItemModel::rowsRemoved, this,
          [this]() { update_actions(this); });
  connect(chords_model_pointer, &QAbstractItemModel::modelReset, this,
          [this]() { update_actions(this); });

  const auto *primary_screen_pointer = QGuiApplication::primaryScreen();
  Q_ASSERT(primary_screen_pointer != nullptr);
  resize(sizeHint().width(),
         primary_screen_pointer->availableGeometry().height());

  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time(this);
}

SongEditor::~SongEditor() {
  undo_stack_pointer->disconnect();

  delete_audio_driver(this);
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

void set_gain_directly(const SongEditor *song_editor_pointer, double new_gain) {
  Q_ASSERT(song_editor_pointer != nullptr);
  set_value_no_signals(song_editor_pointer->gain_editor_pointer, new_gain);
  song_editor_pointer->chords_model_pointer->gain = new_gain;
  fluid_synth_set_gain(song_editor_pointer->synth_pointer,
                       static_cast<float>(new_gain));
}

void set_starting_key_directly(const SongEditor *song_editor_pointer,
                               double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  set_value_no_signals(song_editor_pointer->starting_key_editor_pointer,
                       new_value);
  song_editor_pointer->chords_model_pointer->starting_key = new_value;
}

void set_starting_velocity_directly(const SongEditor *song_editor_pointer,
                                    double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  set_value_no_signals(song_editor_pointer->starting_velocity_editor_pointer,
                       new_value);
  song_editor_pointer->chords_model_pointer->starting_velocity = new_value;
}

void set_starting_tempo_directly(const SongEditor *song_editor_pointer,
                                 double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  set_value_no_signals(song_editor_pointer->starting_tempo_editor_pointer,
                       new_value);
  song_editor_pointer->chords_model_pointer->starting_tempo = new_value;
}

void start_real_time(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);

  auto *settings_pointer = song_editor_pointer->settings_pointer;

  auto default_driver_pointer = std::make_unique<char *>();
  fluid_settings_dupstr(settings_pointer, "audio.driver",
                        default_driver_pointer.get());
  Q_ASSERT(default_driver_pointer != nullptr);

  delete_audio_driver(song_editor_pointer);

  QString default_driver(*default_driver_pointer);

#ifdef __linux__
  fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#else
  fluid_settings_setstr(settings_pointer, "audio.driver",
                        default_driver.c_str());
#endif

#ifndef NO_REALTIME_AUDIO
  song_editor_pointer->audio_driver_pointer = new_fluid_audio_driver(
      settings_pointer, song_editor_pointer->synth_pointer);
#endif
  if (song_editor_pointer->audio_driver_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Cannot start audio driver \"") << default_driver
           << SongEditor::tr("\"");
#ifdef NO_WARN_AUDIO
    qWarning("%s", message.toStdString().c_str());
#else
    QMessageBox::warning(song_editor_pointer,
                         SongEditor::tr("Audio driver error"), message);
#endif
  }
}

void initialize_play(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);

  song_editor_pointer->current_key = get_starting_key(song_editor_pointer);
  song_editor_pointer->current_velocity =
      get_starting_velocity(song_editor_pointer);
  song_editor_pointer->current_tempo = get_starting_tempo(song_editor_pointer);

  auto current_time =
      fluid_sequencer_get_tick(song_editor_pointer->sequencer_pointer);

  song_editor_pointer->starting_time = current_time;
  song_editor_pointer->current_time = current_time;
  song_editor_pointer->final_time = current_time;

  auto &channel_schedules = song_editor_pointer->channel_schedules;
  for (size_t index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = current_time;
  }
}

void send_event_at(fluid_sequencer_t *sequencer_pointer,
                   fluid_event_t *event_pointer, double time) {
  Q_ASSERT(time >= 0);
  auto result =
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1);
  Q_ASSERT(result == FLUID_OK);
};

void play_chords(SongEditor *song_editor_pointer, size_t first_chord_number,
                 size_t number_of_chords, int wait_frames) {
  Q_ASSERT(song_editor_pointer != nullptr);

  const auto &chords = song_editor_pointer->chords_model_pointer->chords;

  auto start_time = song_editor_pointer->current_time + wait_frames;
  song_editor_pointer->current_time = start_time;
  update_final_time(song_editor_pointer, start_time);
  for (auto chord_index = first_chord_number;
       chord_index < first_chord_number + number_of_chords;
       chord_index = chord_index + 1) {
    const auto &chord = get_const_item(chords, chord_index);

    modulate(song_editor_pointer, chord);
    play_notes(song_editor_pointer, chord_index, chord, 0, chord.notes.size());
    auto new_current_time = song_editor_pointer->current_time +
                            (get_beat_time(song_editor_pointer->current_tempo) *
                             rational_to_double(chord.beats)) *
                                MILLISECONDS_PER_SECOND;
    song_editor_pointer->current_time = new_current_time;
    update_final_time(song_editor_pointer, new_current_time);
  }
}

void stop_playing(fluid_sequencer_t *sequencer_pointer,
                  fluid_event_t *event_pointer) {
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

void delete_audio_driver(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);

  auto *audio_driver_pointer = song_editor_pointer->audio_driver_pointer;

  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
    song_editor_pointer->audio_driver_pointer = nullptr;
  }
}