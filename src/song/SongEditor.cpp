// TODO: add percussion tests

#include "song/SongEditor.hpp"

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
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iterator>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <qabstractitemmodel.h>
#include <qitemselectionmodel.h>
#include <qspinbox.h>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "chord/InsertChord.hpp"
#include "chord/RemoveChords.hpp"
#include "chord/SetChordsCells.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "justly/justly.hpp"
#include "note/InsertNote.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"
#include "note/RemoveNotes.hpp"
#include "note/SetNotesCells.hpp"
#include "other/JustlyView.hpp"
#include "other/bounds.hpp"
#include "other/conversions.hpp"
#include "percussion/InsertPercussion.hpp"
#include "percussion/PercussionsModel.hpp"
#include "percussion/RemovePercussions.hpp"
#include "percussion/SetPercussionsCells.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "rational/Rational.hpp"
#include "song/SetGain.hpp"
#include "song/SetStartingKey.hpp"
#include "song/SetStartingTempo.hpp"
#include "song/SetStartingVelocity.hpp"
#include "song/EditNotes.hpp"
#include "song/EditPercussions.hpp"
#include "song/NotesToChords.hpp"
#include "song/PercussionsToChords.hpp"

static const auto NUMBER_OF_MIDI_CHANNELS = 16;

static const auto GAIN_STEP = 0.1;

static const auto SECONDS_PER_MINUTE = 60;
static const unsigned int MILLISECONDS_PER_SECOND = 1000;

static const auto BEND_PER_HALFSTEP = 4096;
static const auto ZERO_BEND_HALFSTEPS = 2;

static const auto VERBOSE_FLUIDSYNTH = false;

static const auto CHORDS_MIME = "application/prs.chords+json";
static const auto NOTES_MIME = "application/prs.notes+json";
static const auto CHORDS_CELLS_MIME = "application/prs.chords_cells+json";
static const auto NOTES_CELLS_MIME = "application/prs.notes_cells+json";
static const auto PERCUSSIONS_CELLS_MIME =
    "application/prs.percussions_cells+json";

[[nodiscard]] auto get_beat_time(double tempo) -> double {
  return SECONDS_PER_MINUTE / tempo;
}

static auto get_only_range(const QTableView *table_view_pointer)
    -> const QItemSelectionRange & {
  const auto *selection_model_pointer = table_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  const auto &selection = selection_model_pointer->selection();
  Q_ASSERT(selection.size() == 1);
  return selection.at(0);
}

static void set_model(SongEditor *song_editor_pointer,
                      QAbstractItemModel *model_pointer) {
  auto *table_view_pointer = song_editor_pointer->table_view_pointer;
  table_view_pointer->setModel(model_pointer);
  const auto *selection_model_pointer = table_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  SongEditor::connect(selection_model_pointer,
                      &QItemSelectionModel::selectionChanged,
                      song_editor_pointer, &SongEditor::update_actions);
  song_editor_pointer->update_actions();
}

[[nodiscard]] static auto get_settings_pointer() -> fluid_settings_t * {
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

[[nodiscard]] static auto
rational_to_double(const Rational &rational) -> double {
  Q_ASSERT(rational.denominator != 0);
  return 1.0 * rational.numerator / rational.denominator;
}

static void copy_json(const nlohmann::json &copied, const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  new_data_pointer->setData(mime_type, json_text.str().c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

[[nodiscard]] static auto
get_mime_description(const QString &mime_type) -> QString {
  if (mime_type == CHORDS_MIME) {
    return ChordsModel::tr("chords");
  }
  if (mime_type == NOTES_MIME) {
    return ChordsModel::tr("notes");
  }
  if (mime_type == CHORDS_CELLS_MIME) {
    return ChordsModel::tr("chords cells");
  }
  if (mime_type == NOTES_CELLS_MIME) {
    return ChordsModel::tr("notes cells");
  }
  if (mime_type == PERCUSSIONS_CELLS_MIME) {
    return ChordsModel::tr("percussions cells");
  }
  return mime_type;
}

[[nodiscard]] static auto parse_clipboard(
    QWidget *parent_pointer, const QString &mime_type,
    const nlohmann::json_schema::json_validator &validator) -> nlohmann::json {
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
    return {};
  }
  const auto &copied_text = mime_data_pointer->data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Parsing error"),
                         parse_error.what());
    return {};
  }
  if (copied.empty()) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Empty paste"),
                         "Nothing to paste!");
    return {};
  }
  try {
    validator.validate(copied);
  } catch (const std::exception &error) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Schema error"),
                         error.what());
    return {};
  }
  return copied;
}

[[nodiscard]] static auto
get_chord_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", chord_interval_column},
                         {"maximum", chord_words_column}});
}

[[nodiscard]] static auto
get_note_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", note_instrument_column},
                         {"maximum", note_words_column}});
}

[[nodiscard]] static auto
get_percussion_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", percussion_set_column},
                         {"maximum", percussion_tempo_ratio_column}});
}

[[nodiscard]] static auto
get_rational_schema(const char *description) -> nlohmann::json {
  return nlohmann::json(
      {{"type", "object"},
       {"description", description},
       {"properties",
        nlohmann::json(
            {{"numerator",
              nlohmann::json({{"type", "integer"},
                              {"description", "the numerator"},
                              {"minimum", 1},
                              {"maximum", MAX_RATIONAL_NUMERATOR}})},
             {"denominator",
              nlohmann::json({{"type", "integer"},
                              {"description", "the denominator"},
                              {"minimum", 1},
                              {"maximum", MAX_RATIONAL_DENOMINATOR}})}})}});
}

[[nodiscard]] static auto get_words_schema() -> nlohmann::json {
  return nlohmann::json({{"type", "string"}, {"description", "the words"}});
}

template <typename Item> static auto get_names(const QList<Item> &items) {
  std::vector<std::string> names;
  std::transform(
      items.cbegin(), items.cend(), std::back_inserter(names),
      [](const Item &item) -> std::string { return item.name.toStdString(); });
  return names;
}

[[nodiscard]] static auto get_notes_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", "the notes"},
       {"items",
        nlohmann::json(
            {{"type", "object"},
             {"description", "a note"},
             {"properties",
              nlohmann::json(
                  {{"instrument",
                    nlohmann::json(
                        {{"type", "string"},
                         {"description", "the instrument"},
                         {"enum", get_names(get_all_instruments())}})},
                   {"interval",
                    nlohmann::json(
                        {{"type", "object"},
                         {"description", "an interval"},
                         {"properties",
                          nlohmann::json(
                              {{"numerator",
                                {{"type", "integer"},
                                 {"description", "the numerator"},
                                 {"minimum", 1},
                                 {"maximum", MAX_INTERVAL_NUMERATOR}}},
                               {"denominator",
                                nlohmann::json(
                                    {{"type", "integer"},
                                     {"description", "the denominator"},
                                     {"minimum", 1},
                                     {"maximum", MAX_INTERVAL_DENOMINATOR}})},
                               {"octave", nlohmann::json(
                                              {{"type", "integer"},
                                               {"description", "the octave"},
                                               {"minimum", MIN_OCTAVE},
                                               {"maximum", MAX_OCTAVE}})}})}})},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_percent", get_rational_schema("velocity ratio")},
                   {"tempo_percent", get_rational_schema("tempo ratio")},
                   {"words", get_words_schema()}})}})}});
}

[[nodiscard]] static auto get_percussions_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", "the percussions"},
       {"items",
        nlohmann::json(
            {{"type", "object"},
             {"description", "a percussion"},
             {"properties",
              nlohmann::json(
                  {{"percussion_set",
                    nlohmann::json(
                        {{"type", "string"},
                         {"description", "the percussion set"},
                         {"enum", get_names(get_all_percussion_sets())}})},
                   {"percussion_instrument",
                    nlohmann::json(
                        {{"type", "string"},
                         {"description", "the percussion instrument"},
                         {"enum",
                          get_names(get_all_percussion_instruments())}})},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_percent", get_rational_schema("velocity ratio")},
                   {"tempo_percent",
                    get_rational_schema("tempo ratio")}})}})}});
}

static auto
get_first_child_number(const QItemSelectionRange &range) -> qsizetype {
  return to_qsizetype(range.top());
}

static auto
get_number_of_children(const QItemSelectionRange &range) -> qsizetype {
  return to_qsizetype(range.bottom() - range.top() + 1);
}

void SongEditor::copy_selected() const {
  const auto &range = get_only_range(table_view_pointer);
  auto first_child_number = get_first_child_number(range);
  auto number_of_children = get_number_of_children(range);
  auto left_column = range.left();
  auto right_column = range.right();

  if (current_model_type == chords_type) {
    copy_json(
        nlohmann::json({{"left_column", to_chord_column(left_column)},
                        {"right_column", to_chord_column(right_column)},
                        {"chords", chords_to_json(chords_model_pointer->chords,
                                                  first_child_number,
                                                  number_of_children, false)}}),
        CHORDS_CELLS_MIME);
  } else if (current_model_type == notes_type) {
    auto *notes_pointer = notes_model_pointer->notes_pointer;
    Q_ASSERT(notes_pointer != nullptr);
    copy_json(nlohmann::json(
                  {{"left_column", to_note_column(left_column)},
                   {"right_column", to_note_column(right_column)},
                   {"notes", notes_to_json(*notes_pointer, first_child_number,
                                           number_of_children)}}),
              NOTES_CELLS_MIME);
  } else {
    auto *percussions_pointer = percussions_model_pointer->percussions_pointer;
    Q_ASSERT(percussions_pointer != nullptr);
    copy_json(nlohmann::json(
                  {{"left_column", to_percussion_column(left_column)},
                   {"right_column", to_percussion_column(right_column)},
                   {"percussions", percussions_to_json((*percussions_pointer),
                                                       first_child_number,
                                                       number_of_children)}}),
              PERCUSSIONS_CELLS_MIME);
  }
}

static void insert_row(const SongEditor *song_editor_pointer,
                       qsizetype child_number) {
  auto current_model_type = song_editor_pointer->current_model_type;
  auto *undo_stack_pointer = song_editor_pointer->undo_stack_pointer;
  if (current_model_type == chords_type) {
    undo_stack_pointer->push(
        std::make_unique<InsertChord>(song_editor_pointer->chords_model_pointer,
                                      child_number, Chord())
            .release());
  } else if (current_model_type == notes_type) {
    undo_stack_pointer->push(
        std::make_unique<InsertNote>(song_editor_pointer->notes_model_pointer,
                                     child_number, Note())
            .release());
  } else {
    undo_stack_pointer->push(std::make_unique<InsertPercussion>(
                                 song_editor_pointer->percussions_model_pointer,
                                 child_number, Percussion())
                                 .release());
  }
}

template <typename Item>
static auto copy_items(const QList<Item> &items, qsizetype first_child_number,
                       qsizetype number_of_children) -> QList<Item> {
  QList<Item> copied;
  std::copy(items.cbegin() + first_child_number,
            items.cbegin() + first_child_number + number_of_children,
            std::back_inserter(copied));
  return copied;
}

void SongEditor::delete_selected() {
  const auto &range = get_only_range(table_view_pointer);
  auto first_child_number = get_first_child_number(range);
  auto number_of_children = get_number_of_children(range);
  auto left_column = range.left();
  auto right_column = range.right();

  if (current_model_type == chords_type) {
    undo_stack_pointer->push(
        std::make_unique<SetChordsCells>(
            chords_model_pointer, first_child_number,
            to_chord_column(left_column), to_chord_column(right_column),
            copy_items(chords_model_pointer->chords, first_child_number,
                       number_of_children),
            QList<Chord>(number_of_children))
            .release());
  } else if (current_model_type == notes_type) {
    auto *notes_pointer = notes_model_pointer->notes_pointer;
    Q_ASSERT(notes_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<SetNotesCells>(
            notes_model_pointer, first_child_number,
            to_note_column(left_column), to_note_column(right_column),
            copy_items(*notes_pointer, first_child_number, number_of_children),
            QList<Note>(number_of_children))
            .release());
  } else {
    auto *percussions_pointer = percussions_model_pointer->percussions_pointer;
    Q_ASSERT(percussions_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<SetPercussionsCells>(
            percussions_model_pointer, first_child_number,
            to_percussion_column(left_column),
            to_percussion_column(right_column),
            copy_items((*percussions_pointer), first_child_number,
                       number_of_children),
            QList<Percussion>(number_of_children))
            .release());
  }
}

void SongEditor::remove_rows() {
  const auto &range = get_only_range(table_view_pointer);
  auto first_child_number = get_first_child_number(range);
  auto number_of_children = get_number_of_children(range);

  if (current_model_type == chords_type) {
    undo_stack_pointer->push(
        std::make_unique<RemoveChords>(chords_model_pointer, first_child_number,
                                       copy_items(chords_model_pointer->chords,
                                                  first_child_number,
                                                  number_of_children))
            .release());
  } else if (current_model_type == notes_type) {
    auto *notes_pointer = notes_model_pointer->notes_pointer;
    Q_ASSERT(notes_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<RemoveNotes>(
            notes_model_pointer, first_child_number,
            copy_items(*notes_pointer, first_child_number, number_of_children))
            .release());
  } else {
    auto *percussions_pointer = percussions_model_pointer->percussions_pointer;
    Q_ASSERT(percussions_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<RemovePercussions>(
            percussions_model_pointer, first_child_number,
            copy_items((*percussions_pointer), first_child_number,
                       number_of_children))
            .release());
  }
}

static auto make_file_dialog(SongEditor *song_editor_pointer,
                             const QString &caption, const QString &filter,
                             QFileDialog::AcceptMode accept_mode,
                             const QString &suffix,
                             QFileDialog::FileMode file_mode) -> QFileDialog * {
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
                              QFileDialog *dialog_pointer) -> QString {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->current_folder =
      dialog_pointer->directory().absolutePath();
  const auto &selected_files = dialog_pointer->selectedFiles();
  Q_ASSERT(!(selected_files.empty()));
  return selected_files[0];
}

static void modulate(SongEditor *song_editor_pointer, const Chord &chord) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->current_key =
      song_editor_pointer->current_key * interval_to_double(chord.interval);
  song_editor_pointer->current_velocity =
      song_editor_pointer->current_velocity *
      rational_to_double(chord.velocity_ratio);
  song_editor_pointer->current_tempo = song_editor_pointer->current_tempo *
                                       rational_to_double(chord.tempo_ratio);
}

static void update_final_time(SongEditor *song_editor_pointer,
                              double new_final_time) {
  Q_ASSERT(song_editor_pointer != nullptr);
  if (new_final_time > song_editor_pointer->final_time) {
    song_editor_pointer->final_time = new_final_time;
  }
};

static auto
get_open_channel_number(SongEditor *song_editor_pointer, int chord_index,
                        int item_number,
                        const QString &item_description) -> qsizetype {
  const auto &channel_schedules = song_editor_pointer->channel_schedules;
  auto current_time = song_editor_pointer->current_time;
  for (qsizetype channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
       channel_index = channel_index + 1) {
    if (current_time >= channel_schedules.at(channel_index)) {
      return channel_index;
    }
  }
  QString message;
  QTextStream stream(&message);
  stream << SongEditor::tr("Out of MIDI channels for chord ") << chord_index + 1
         << SongEditor::tr(", ") << item_description << " " << item_number + 1
         << SongEditor::tr(". Not playing ") << item_description << ".";
  QMessageBox::warning(song_editor_pointer,
                       SongEditor::tr("MIDI channel error"), message);
  return -1;
}

static void play_note(SongEditor *song_editor_pointer, int channel_number,
                      int16_t midi_number, const Rational &beats,
                      const Rational &velocity_ratio,
                      const Rational &tempo_ratio, int time_offset,
                      int chord_index, int item_number,
                      const QString &item_description) {
  auto velocity = song_editor_pointer->current_velocity *
                  rational_to_double(velocity_ratio);
  auto *event_pointer = song_editor_pointer->event_pointer;
  int16_t new_velocity = 1;
  if (velocity > MAX_VELOCITY) {
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Velocity ") << velocity
           << SongEditor::tr(" exceeds ") << MAX_VELOCITY
           << SongEditor::tr(" for chord ") << chord_index + 1
           << SongEditor::tr(", ") << item_description << " " << item_number + 1
           << SongEditor::tr(". Playing with velocity ") << MAX_VELOCITY
           << SongEditor::tr(".");
    QMessageBox::warning(song_editor_pointer, SongEditor::tr("Velocity error"),
                         message);
  } else {
    new_velocity = static_cast<int16_t>(std::round(velocity));
  }
  fluid_event_noteon(event_pointer, channel_number, midi_number, new_velocity);
  send_event_at(song_editor_pointer->sequencer_pointer, event_pointer,
                song_editor_pointer->current_time + time_offset);

  auto end_time = song_editor_pointer->current_time +
                  get_beat_time(song_editor_pointer->current_tempo *
                                rational_to_double(tempo_ratio)) *
                      rational_to_double(beats) * MILLISECONDS_PER_SECOND;

  auto &channel_schedules = song_editor_pointer->channel_schedules;
  fluid_event_noteoff(event_pointer, channel_number, midi_number);
  send_event_at(song_editor_pointer->sequencer_pointer, event_pointer,
                end_time);
  Q_ASSERT(channel_number < channel_schedules.size());
  channel_schedules[channel_number] = end_time;
  update_final_time(song_editor_pointer, end_time);
}

static void change_instrument(SongEditor *song_editor_pointer,
                              int channel_number, int16_t bank_number,
                              int16_t preset_number) {
  auto *event_pointer = song_editor_pointer->event_pointer;
  fluid_event_program_select(event_pointer, channel_number,
                             song_editor_pointer->soundfont_id, bank_number,
                             preset_number);
  send_event_at(song_editor_pointer->sequencer_pointer, event_pointer,
                song_editor_pointer->current_time);
}

static void play_notes(SongEditor *song_editor_pointer, qsizetype chord_index,
                       const Chord &chord, qsizetype first_note_index,
                       qsizetype number_of_notes) {
  Q_ASSERT(song_editor_pointer != nullptr);

  auto current_time = song_editor_pointer->current_time;
  auto current_key = song_editor_pointer->current_key;

  auto *sequencer_pointer = song_editor_pointer->sequencer_pointer;
  auto *event_pointer = song_editor_pointer->event_pointer;

  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    auto channel_number = get_open_channel_number(
        song_editor_pointer, chord_index, note_index, SongEditor::tr("note"));
    if (channel_number != -1) {
      const auto &note = chord.notes.at(note_index);

      const auto *instrument_pointer = note.instrument_pointer;
      Q_ASSERT(instrument_pointer != nullptr);
      change_instrument(song_editor_pointer, channel_number,
                        instrument_pointer->bank_number,
                        instrument_pointer->preset_number);

      auto midi_float =
          get_midi(current_key * interval_to_double(note.interval));
      auto closest_midi = round(midi_float);
      auto midi_number = static_cast<int16_t>(closest_midi);

      fluid_event_pitch_bend(
          event_pointer, channel_number,
          static_cast<int>(
              std::round((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
                         BEND_PER_HALFSTEP)));
      send_event_at(sequencer_pointer, event_pointer, current_time + 1);

      play_note(song_editor_pointer, channel_number, midi_number, note.beats,
                note.velocity_ratio, note.tempo_ratio, 2, chord_index,
                note_index, SongEditor::tr("note"));
    }
  }
}

static void play_percussions(SongEditor *song_editor_pointer,
                             qsizetype chord_index, const Chord &chord,
                             qsizetype first_percussion_number,
                             qsizetype number_of_percussions) {
  Q_ASSERT(song_editor_pointer != nullptr);
  for (auto percussion_number = first_percussion_number;
       percussion_number < first_percussion_number + number_of_percussions;
       percussion_number = percussion_number + 1) {
    auto channel_number = get_open_channel_number(
        song_editor_pointer, chord_index, percussion_number,
        SongEditor::tr("percussion"));
    if (channel_number != -1) {
      const auto &percussion = chord.percussions.at(percussion_number);

      const auto *percussion_set_pointer = percussion.percussion_set_pointer;
      Q_ASSERT(percussion_set_pointer != nullptr);

      change_instrument(song_editor_pointer, channel_number,
                        percussion_set_pointer->bank_number,
                        percussion_set_pointer->preset_number);

      const auto *percussion_instrument_pointer =
          percussion.percussion_instrument_pointer;
      Q_ASSERT(percussion_instrument_pointer != nullptr);

      play_note(song_editor_pointer, channel_number,
                percussion_instrument_pointer->midi_number, percussion.beats,
                percussion.velocity_ratio, percussion.tempo_ratio, 1,
                chord_index, percussion_number, SongEditor::tr("percussion"));
    }
  }
}

void SongEditor::update_actions() const {
  auto is_chords = current_model_type == chords_type;
  auto *selection_model_pointer = table_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);

  auto anything_selected = false;
  auto one_row_selected = false;
  const auto &selection = selection_model_pointer->selection();
  auto selection_size = selection.size();
  if (selection_size > 0) {
    anything_selected = true;
    const auto &range = get_only_range(table_view_pointer);
    one_row_selected = range.top() == range.bottom();
  }
  auto one_chord_selected = is_chords && one_row_selected;
  cut_action_pointer->setEnabled(anything_selected);
  copy_action_pointer->setEnabled(anything_selected);
  insert_after_action_pointer->setEnabled(anything_selected);
  delete_action_pointer->setEnabled(anything_selected);
  remove_rows_action_pointer->setEnabled(anything_selected);
  play_action_pointer->setEnabled(anything_selected);
  paste_action_pointer->setEnabled(anything_selected);
  edit_notes_action_pointer->setEnabled(one_chord_selected);
  edit_percussions_action_pointer->setEnabled(one_chord_selected);
  edit_chords_action_pointer->setEnabled(!is_chords);
}

[[nodiscard]] auto make_validator(const char *title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return {json};
}

auto get_chords_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", "a list of chords"},
       {"items",
        nlohmann::json(
            {{"type", "object"},
             {"description", "a chord"},
             {"properties",
              nlohmann::json(
                  {{"interval",
                    nlohmann::json(
                        {{"type", "object"},
                         {"description", "an interval"},
                         {"properties",
                          nlohmann::json(
                              {{"numerator",
                                nlohmann::json(
                                    {{"type", "integer"},
                                     {"description", "the numerator"},
                                     {"minimum", 1},
                                     {"maximum", MAX_INTERVAL_NUMERATOR}})},
                               {"denominator",
                                nlohmann::json(
                                    {{"type", "integer"},
                                     {"description", "the denominator"},
                                     {"minimum", 1},
                                     {"maximum", MAX_INTERVAL_DENOMINATOR}})},
                               {"octave",
                                nlohmann::json({{"type", "integer"},
                                                {"description", "the octave"},
                                                {"minimum", MIN_OCTAVE},
                                                {"maximum", MAX_OCTAVE}})},
                               {"beats",
                                get_rational_schema("the number of beats")},
                               {"velocity_percent",
                                get_rational_schema("velocity ratio")},
                               {"tempo_percent",
                                get_rational_schema("tempo ratio")},
                               {"words", get_words_schema()},
                               {"notes", get_notes_schema()}})}})}})}})}});
}

void SongEditor::play() {
  const auto &chords = chords_model_pointer->chords;

  const auto &range = get_only_range(table_view_pointer);
  auto first_child_number = get_first_child_number(range);
  auto number_of_children = get_number_of_children(range);

  stop_playing();
  initialize_play(this);
  if (current_model_type == chords_type) {
    if (first_child_number > 0) {
      for (qsizetype chord_index = 0; chord_index < first_child_number;
           chord_index = chord_index + 1) {
        modulate(this, chords.at(chord_index));
      }
    }
    play_chords(this, first_child_number, number_of_children);
  } else if (current_model_type == notes_type) {
    for (qsizetype chord_index = 0; chord_index <= current_chord_number;
         chord_index = chord_index + 1) {
      modulate(this, chords.at(chord_index));
    }
    play_notes(this, current_chord_number, chords.at(current_chord_number),
               first_child_number, number_of_children);
  } else {
    for (qsizetype chord_index = 0; chord_index <= current_chord_number;
         chord_index = chord_index + 1) {
      modulate(this, chords.at(chord_index));
    }
    play_percussions(this, current_chord_number,
                     chords.at(current_chord_number), first_child_number,
                     number_of_children);
  }
}

void SongEditor::save() { save_as_file(this, current_file); }

void SongEditor::cut() {
  copy_selected();
  delete_selected();
}

void SongEditor::export_wav() {
  auto *dialog_pointer =
      make_file_dialog(this, tr("Export — Justly"), "WAV file (*.wav)",
                       QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
  dialog_pointer->setLabelText(QFileDialog::Accept, "Export");
  if (dialog_pointer->exec() != 0) {
    export_to_file(this, get_selected_file(this, dialog_pointer));
  }
}

void SongEditor::open() {
  if (undo_stack_pointer->isClean() || ask_discard_changes(this)) {
    auto *dialog_pointer = make_file_dialog(
        this, tr("Open — Justly"), "JSON file (*.json)",
        QFileDialog::AcceptOpen, ".json", QFileDialog::ExistingFile);
    if (dialog_pointer->exec() != 0) {
      open_file(this, get_selected_file(this, dialog_pointer));
    }
  }
}

void SongEditor::save_as() {
  auto *dialog_pointer =
      make_file_dialog(this, tr("Export — Justly"), "WAV file (*.wav)",
                       QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
  dialog_pointer->setLabelText(QFileDialog::Accept, "Export");
  if (dialog_pointer->exec() != 0) {
    export_to_file(this, get_selected_file(this, dialog_pointer));
  }
}

void SongEditor::insert_after() const {
  insert_row(this, get_only_range(table_view_pointer).bottom() + 1);
}

static auto get_int(const nlohmann::json &json_data, const char *field) {
  Q_ASSERT(json_data.contains(field));
  const auto &json_value = json_data[field];
  Q_ASSERT(json_value.is_number());
  return json_value.get<int>();
}

void SongEditor::paste_cells() {
  auto first_child_number =
      get_first_child_number(get_only_range(table_view_pointer));

  if (current_model_type == chords_type) {
    const auto &chords = chords_model_pointer->chords;
    static const nlohmann::json_schema::json_validator chords_cells_validator =
        make_validator(
            "Chords cells",
            nlohmann::json(
                {{"description", "chords cells"},
                 {"type", "object"},
                 {"required", {"left_column", "right_column", "chords"}},
                 {"properties",
                  nlohmann::json({{"left_column",
                                   get_chord_column_schema("left ChordColumn")},
                                  {"right_column", get_chord_column_schema(
                                                       "right ChordColumn")},
                                  {"chords", get_chords_schema()}})}}));
    const auto json_chords_cells =
        parse_clipboard(chords_model_pointer->parent_pointer, CHORDS_CELLS_MIME,
                        chords_cells_validator);
    if (json_chords_cells.empty()) {
      return;
    }

    Q_ASSERT(json_chords_cells.contains("chords"));
    const auto &json_chords = json_chords_cells["chords"];

    auto number_of_chords =
        std::min({static_cast<qsizetype>(json_chords.size()),
                  chords.size() - first_child_number});

    QList<Chord> new_chords;
    json_to_chords(new_chords, json_chords, number_of_chords);
    undo_stack_pointer->push(
        std::make_unique<SetChordsCells>(
            chords_model_pointer, first_child_number,
            to_chord_column(get_int(json_chords_cells, "left_column")),
            to_chord_column(get_int(json_chords_cells, "right_column")),
            copy_items(chords, first_child_number, number_of_chords),
            std::move(new_chords))
            .release());
  } else if (current_model_type == notes_type) {
    auto *notes_pointer = notes_model_pointer->notes_pointer;
    Q_ASSERT(notes_pointer != nullptr);
    static const nlohmann::json_schema::json_validator notes_cells_validator =
        make_validator(
            "Notes cells",
            nlohmann::json(
                {{"description", "notes cells"},
                 {"type", "object"},
                 {"required", {"left_column", "right_column", "notes"}},
                 {"properties",
                  nlohmann::json({{"left_column",
                                   get_note_column_schema("left note column")},
                                  {"right_column",
                                   get_note_column_schema("right note column")},
                                  {"notes", get_notes_schema()}})}}));
    const auto json_notes_cells =
        parse_clipboard(notes_model_pointer->parent_pointer, NOTES_CELLS_MIME,
                        notes_cells_validator);
    if (json_notes_cells.empty()) {
      return;
    }

    Q_ASSERT(json_notes_cells.contains("notes"));
    const auto &json_notes = json_notes_cells["notes"];

    auto number_of_notes =
        std::min({static_cast<qsizetype>(json_notes.size()),
                  notes_pointer->size() - first_child_number});
    QList<Note> new_notes;
    json_to_notes(new_notes, json_notes, number_of_notes);
    undo_stack_pointer->push(
        std::make_unique<SetNotesCells>(
            notes_model_pointer, first_child_number,
            to_note_column(get_int(json_notes_cells, "left_column")),
            to_note_column(get_int(json_notes_cells, "right_column")),
            copy_items(*notes_pointer, first_child_number, number_of_notes),
            std::move(new_notes))
            .release());
  } else {
    auto *percussions_pointer = percussions_model_pointer->percussions_pointer;
    Q_ASSERT(percussions_pointer != nullptr);
    static const nlohmann::json_schema::json_validator
        percussions_cells_validator = make_validator(
            "Percussions cells",
            nlohmann::json(
                {{"description", "cells"},
                 {"type", "object"},
                 {"required", {"left_column", "right_column", "percussions"}},
                 {"properties",
                  nlohmann::json(
                      {{"left_column",
                        get_percussion_column_schema("left percussion column")},
                       {"right_column", get_percussion_column_schema(
                                            "right percussion column")},
                       {"percussions", get_percussions_schema()}})}}));
    const auto json_percussions_cells =
        parse_clipboard(percussions_model_pointer->parent_pointer,
                        PERCUSSIONS_CELLS_MIME, percussions_cells_validator);
    if (json_percussions_cells.empty()) {
      return;
    }

    Q_ASSERT(json_percussions_cells.contains("chords"));
    const auto &json_percussions = json_percussions_cells["chords"];

    auto number_of_percurussions =
        std::min({static_cast<qsizetype>(json_percussions.size()),
                  percussions_pointer->size() - first_child_number});

    QList<Percussion> new_percussions;
    json_to_percussions(new_percussions, json_percussions,
                        number_of_percurussions);
    undo_stack_pointer->push(
        std::make_unique<SetPercussionsCells>(
            percussions_model_pointer, first_child_number,
            to_percussion_column(
                get_int(json_percussions_cells, "left_column")),
            to_percussion_column(
                get_int(json_percussions_cells, "right_column")),
            copy_items((*percussions_pointer), first_child_number,
                       number_of_percurussions),
            std::move(new_percussions))
            .release());
  }
}

void SongEditor::insert_into() const { insert_row(this, 0); }

void SongEditor::edit_chords() {
  if (current_model_type == notes_type) {
    undo_stack_pointer->push(
      std::make_unique<NotesToChords>(this, current_chord_number)
          .release());
  } else if (current_model_type == percussion_type) {
    undo_stack_pointer->push(
      std::make_unique<PercussionsToChords>(this, current_chord_number)
          .release());
  } else {
    Q_ASSERT(false);
  }
}

void SongEditor::edit_notes() {
  undo_stack_pointer->push(
      std::make_unique<EditNotes>(this, get_only_range(table_view_pointer).top())
          .release());
}

void notes_to_chords(SongEditor *song_editor_pointer) {
  auto *notes_model_pointer = song_editor_pointer->notes_model_pointer;
  set_model(song_editor_pointer, song_editor_pointer->chords_model_pointer);
  notes_model_pointer->begin_reset_model();
  notes_model_pointer->notes_pointer = nullptr;
  notes_model_pointer->end_reset_model();
  song_editor_pointer->current_model_type = chords_type;
  song_editor_pointer->current_chord_number = -1;
}

void percussions_to_chords(SongEditor *song_editor_pointer) {
  auto *percussions_model_pointer =
      song_editor_pointer->percussions_model_pointer;
  set_model(song_editor_pointer, song_editor_pointer->chords_model_pointer);
  percussions_model_pointer->begin_reset_model();
  percussions_model_pointer->percussions_pointer = nullptr;
  percussions_model_pointer->end_reset_model();
  song_editor_pointer->current_model_type = chords_type;
  song_editor_pointer->current_chord_number = -1;
}

void edit_notes_directly(SongEditor *song_editor_pointer,
                         qsizetype chord_number) {
  Q_ASSERT(song_editor_pointer != nullptr);
  auto *notes_model_pointer = song_editor_pointer->notes_model_pointer;
  Q_ASSERT(song_editor_pointer->current_model_type == chords_type);
  Q_ASSERT(notes_model_pointer->notes_pointer == nullptr);
  song_editor_pointer->current_chord_number = chord_number;
  notes_model_pointer->begin_reset_model();
  notes_model_pointer->notes_pointer =
      &song_editor_pointer->chords_model_pointer->chords[chord_number].notes;
  notes_model_pointer->end_reset_model();
  song_editor_pointer->current_model_type = notes_type;
  set_model(song_editor_pointer, notes_model_pointer);
}

void SongEditor::edit_percussions() {
  undo_stack_pointer->push(
      std::make_unique<EditPercussions>(this, get_only_range(table_view_pointer).top())
          .release());
}

void edit_percussions_directly(SongEditor *song_editor_pointer,
                               qsizetype chord_number) {
  Q_ASSERT(song_editor_pointer->current_model_type == chords_type);
  auto *percussions_model_pointer =
      song_editor_pointer->percussions_model_pointer;
  Q_ASSERT(percussions_model_pointer->percussions_pointer == nullptr);
  song_editor_pointer->current_chord_number = chord_number;
  percussions_model_pointer->begin_reset_model();
  percussions_model_pointer->percussions_pointer =
      &song_editor_pointer->chords_model_pointer->chords[chord_number]
           .percussions;
  percussions_model_pointer->end_reset_model();
  song_editor_pointer->current_model_type = percussion_type;
  set_model(song_editor_pointer, percussions_model_pointer);
}

void SongEditor::set_gain(double new_value) {
  undo_stack_pointer->push(
      std::make_unique<SetGain>(this, chords_model_pointer->gain, new_value)
          .release());
}

void SongEditor::set_starting_key(double new_value) {
  undo_stack_pointer->push(
      std::make_unique<SetStartingKey>(this, get_starting_key(this), new_value)
          .release());
}

void SongEditor::set_starting_velocity(double new_value) {
  undo_stack_pointer->push(std::make_unique<SetStartingVelocity>(
                               this, get_starting_velocity(this), new_value)
                               .release());
}

void SongEditor::set_starting_tempo(double new_value) {
  undo_stack_pointer->push(std::make_unique<SetStartingTempo>(
                               this, get_starting_tempo(this), new_value)
                               .release());
}

void SongEditor::change_clean() const {
  save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                  !current_file.isEmpty());
}

static void connect_model(const QAbstractItemModel *model_pointer,
                          const SongEditor *song_editor_pointer) {
  SongEditor::connect(model_pointer, &QAbstractItemModel::rowsInserted,
                      song_editor_pointer, &SongEditor::update_actions);
  SongEditor::connect(model_pointer, &QAbstractItemModel::rowsRemoved,
                      song_editor_pointer, &SongEditor::update_actions);
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      gain_editor_pointer(new QDoubleSpinBox(this)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_velocity_editor_pointer(new QDoubleSpinBox(this)),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      undo_stack_pointer(new QUndoStack(this)),
      table_view_pointer(new JustlyView(this)),
      chords_model_pointer(new ChordsModel(undo_stack_pointer, this)),
      notes_model_pointer(new NotesModel(undo_stack_pointer, this)),
      percussions_model_pointer(new PercussionsModel(undo_stack_pointer, this)),
      edit_chords_action_pointer(new QAction(tr("&Edit chords"), this)),
      edit_notes_action_pointer(new QAction(tr("&Edit notes"), this)),
      edit_percussions_action_pointer(
          new QAction(tr("&Edit percussions"), this)),
      insert_after_action_pointer(new QAction(tr("&After"), this)),
      insert_into_action_pointer(new QAction(tr("&Into start"), this)),
      delete_action_pointer(new QAction(tr("&Delete"), this)),
      remove_rows_action_pointer(new QAction(tr("&Remove rows"), this)),
      cut_action_pointer(new QAction(tr("&Cut"), this)),
      copy_action_pointer(new QAction(tr("&Copy"), this)),
      paste_action_pointer(new QAction(tr("&Paste"), this)),
      save_action_pointer(new QAction(tr("&Save"), this)),
      play_action_pointer(new QAction(tr("&Play selection"), this)),
      stop_playing_action_pointer(new QAction(tr("&Stop playing"), this)),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      channel_schedules(QList<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      settings_pointer(get_settings_pointer()),
      event_pointer(new_fluid_event()),
      sequencer_pointer(new_fluid_sequencer2(0)),
      synth_pointer(new_fluid_synth(settings_pointer)),
      soundfont_id(get_soundfont_id(synth_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
  set_model(this, chords_model_pointer);
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
  connect(open_action_pointer, &QAction::triggered, this, &SongEditor::open);
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this, &SongEditor::save);
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto *save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this,
          &SongEditor::save_as);
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_action_pointer =
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
  connect(export_action_pointer, &QAction::triggered, this,
          &SongEditor::export_wav);
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
  connect(cut_action_pointer, &QAction::triggered, this, &SongEditor::cut);
  edit_menu_pointer->addAction(cut_action_pointer);

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, this,
          &SongEditor::copy_selected);
  edit_menu_pointer->addAction(copy_action_pointer);

  paste_action_pointer->setEnabled(false);
  connect(paste_action_pointer, &QAction::triggered, this,
          &SongEditor::paste_cells);
  paste_action_pointer->setShortcuts(QKeySequence::Paste);
  edit_menu_pointer->addAction(paste_action_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(insert_after_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_after);
  insert_menu_pointer->addAction(insert_after_action_pointer);

  insert_into_action_pointer->setEnabled(true);
  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  connect(insert_into_action_pointer, &QAction::triggered, this,
          &SongEditor::insert_into);
  insert_menu_pointer->addAction(insert_into_action_pointer);

  delete_action_pointer->setEnabled(false);
  delete_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(delete_action_pointer, &QAction::triggered, this,
          &SongEditor::delete_selected);
  edit_menu_pointer->addAction(delete_action_pointer);

  // TODO: add shortcut
  remove_rows_action_pointer->setEnabled(false);
  connect(remove_rows_action_pointer, &QAction::triggered, this,
          &SongEditor::remove_rows);
  edit_menu_pointer->addAction(remove_rows_action_pointer);

  edit_menu_pointer->addSeparator();

  edit_chords_action_pointer->setEnabled(false);
  connect(edit_chords_action_pointer, &QAction::triggered, this,
          &SongEditor::edit_chords);
  edit_menu_pointer->addAction(edit_chords_action_pointer);

  edit_notes_action_pointer->setEnabled(false);
  connect(edit_notes_action_pointer, &QAction::triggered, this,
          &SongEditor::edit_notes);
  edit_menu_pointer->addAction(edit_notes_action_pointer);

  edit_percussions_action_pointer->setEnabled(false);
  connect(edit_percussions_action_pointer, &QAction::triggered, this,
          &SongEditor::edit_percussions);
  edit_menu_pointer->addAction(edit_percussions_action_pointer);

  menu_bar_pointer->addMenu(edit_menu_pointer);

  auto *view_menu_pointer =
      std::make_unique<QMenu>(tr("&View"), this).release();

  auto *view_controls_checkbox_pointer =
      std::make_unique<QAction>(tr("&Controls"), view_menu_pointer).release();

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
  connect(play_action_pointer, &QAction::triggered, this, &SongEditor::play);
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
          &SongEditor::set_gain);
  set_gain_directly(this, chords_model_pointer->gain);
  controls_form_pointer->addRow(tr("&Gain:"), gain_editor_pointer);

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");

  starting_key_editor_pointer->setValue(get_starting_key(this));

  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_starting_key);
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_velocity_editor_pointer->setMinimum(0);
  starting_velocity_editor_pointer->setMaximum(MAX_VELOCITY);
  starting_velocity_editor_pointer->setDecimals(1);
  starting_velocity_editor_pointer->setValue(get_starting_velocity(this));
  connect(starting_velocity_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_starting_velocity);
  controls_form_pointer->addRow(tr("Starting &velocity:"),
                                starting_velocity_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setValue(get_starting_tempo(this));
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);

  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_starting_tempo);
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  controls_pointer->setLayout(controls_form_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  setWindowTitle("Justly");
  setCentralWidget(table_view_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this,
          &SongEditor::change_clean);

  connect_model(chords_model_pointer, this);
  connect_model(notes_model_pointer, this);
  connect_model(percussions_model_pointer, this);

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

static auto set_value_no_signals(QDoubleSpinBox *spin_box_pointer,
                                 double new_value) {
  spin_box_pointer->blockSignals(true);
  spin_box_pointer->setValue(new_value);
  spin_box_pointer->blockSignals(false);
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
  for (qsizetype index = 0; index < NUMBER_OF_MIDI_CHANNELS;
       index = index + 1) {
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

void play_chords(SongEditor *song_editor_pointer, qsizetype first_chord_number,
                 qsizetype number_of_chords, int wait_frames) {
  Q_ASSERT(song_editor_pointer != nullptr);

  const auto &chords = song_editor_pointer->chords_model_pointer->chords;

  auto start_time = song_editor_pointer->current_time + wait_frames;
  song_editor_pointer->current_time = start_time;
  update_final_time(song_editor_pointer, start_time);
  for (auto chord_index = first_chord_number;
       chord_index < first_chord_number + number_of_chords;
       chord_index = chord_index + 1) {
    const auto &chord = chords.at(chord_index);

    modulate(song_editor_pointer, chord);
    play_notes(song_editor_pointer, chord_index, chord, 0, chord.notes.size());
    play_percussions(song_editor_pointer, chord_index, chord, 0,
                     chord.percussions.size());
    auto new_current_time = song_editor_pointer->current_time +
                            (get_beat_time(song_editor_pointer->current_tempo) *
                             rational_to_double(chord.beats)) *
                                MILLISECONDS_PER_SECOND;
    song_editor_pointer->current_time = new_current_time;
    update_final_time(song_editor_pointer, new_current_time);
  }
}

void SongEditor::stop_playing() const {
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