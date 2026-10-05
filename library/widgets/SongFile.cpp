#include "widgets/SongFile.hpp"

#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>

#include "musicxml/MusicXMLImport.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/WindowBody.hpp"
#include "xml/XMLChildren.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/XMLValidator.hpp"

auto can_discard_changes(WindowBody& window_body) -> bool {
  return window_body.undo_stack.isClean() ||
         QMessageBox::question(&window_body, WindowBody::tr("Unsaved changes"),
                               WindowBody::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
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

  set_xml_double(song_node, "gain", get_gain(window_body.player));
  set_xml_double(song_node, "starting_key", song.starting_key);
  set_xml_double(song_node, "starting_tempo", song.starting_tempo);
  set_xml_double(song_node, "starting_velocity", song.starting_velocity);

  maybe_set_xml_rows(song_node, "chords", song.chords);
  maybe_set_xml_rows(song_node, "pitched_voices", song.pitched_voices);
  maybe_set_xml_rows(song_node, "unpitched_voices", song.unpitched_voices);
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

// warns and returns false unless document was read and matches validator's
// schema
auto check_document(QWidget& parent, XMLDocument& document,
                    XMLValidator& validator, const QString& invalid_message)
    -> bool {
  if (document.internal_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("XML error"),
                         QObject::tr("Invalid XML file"));
    return false;
  }
  if (validate_against_schema(validator, document) != 0) {
    QMessageBox::warning(&parent, QObject::tr("Validation Error"),
                         invalid_message);
    return false;
  }
  return true;
}

// loading a file replaces song.chords wholesale, which would leave
// pitched_notes_model/unpitched_notes_model pointing at destroyed Chord
// members if the switch table was drilled into a chord's notes (mirrors the
// reset that replace_table performs when the user navigates back to chords
// manually)
void reset_switch_table_to_chords(SwitchColumn& switch_column) {
  auto& switch_table = switch_column.switch_table;
  // model resets drop the selection without emitting selectionChanged, so
  // clear it first, while the old rows still exist -- otherwise the piano
  // roll's mirrored selection would outlive them, and its next rebuild (e.g.
  // from loading the file's gain) would look up rows the new song lacks
  get_selection_model(switch_table).clear();
  switch_table.pitched_notes_model.set_rows_pointer();
  switch_table.unpitched_notes_model.set_rows_pointer();
  switch_table.delegate.current_row_type = RowType::chord_type;
  set_model(switch_table, switch_table.chords_model);
  switch_column.editing_text.setText(SwitchColumn::tr("Chords"));
}

// the song was replaced wholesale, so there's nothing to undo back to, and
// nothing unsaved to recover
void finish_loading(WindowBody& window_body) {
  clear_and_clean(window_body.undo_stack);
  remove_recovery_file();
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

template <VoiceInterface SubVoice>
[[nodiscard]] auto check_duplicate_or_empty_voice_names(
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
[[nodiscard]] auto check_note_voices(QWidget& parent,
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

}  // namespace

auto open_file(WindowBody& window_body, const QString& filename) -> bool {
  Q_ASSERT(filename.isValidUtf16());
  auto& spin_boxes = window_body.controls_column.spin_boxes;
  auto& switch_table = window_body.switch_column.switch_table;

  auto document = read_xml_file(filename);
  static XMLValidator song_validator("song.xsd");
  if (!check_document(window_body, document, song_validator,
                      QObject::tr("Invalid song file"))) {
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
  for (auto& field_node : get_xml_children(song_node)) {
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
  switch_table.chords_model.replace_all_rows(std::move(new_chords));
  switch_table.pitched_voices_model.replace_all_rows(
      std::move(new_pitched_voices));
  switch_table.unpitched_voices_model.replace_all_rows(
      std::move(new_unpitched_voices));

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
    }
  }

  window_body.current_file = filename;

  finish_loading(window_body);
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

template <RowInterface SubRow>
void clear_rows(RowsModel<SubRow>& rows_model) {
  const auto number_of_rows = rows_model.rowCount(QModelIndex());
  if (number_of_rows > 0) {
    rows_model.remove_rows(0, number_of_rows);
  }
}

template <VoiceInterface SubVoice>
void add_imported_voices(RowsModel<SubVoice>& voices_model,
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

}  // namespace

auto import_musicxml(WindowBody& window_body, const QString& filename) -> bool {
  auto& switch_table = window_body.switch_column.switch_table;
  auto& chords_model = switch_table.chords_model;
  auto& pitched_voices_model = switch_table.pitched_voices_model;
  auto& unpitched_voices_model = switch_table.unpitched_voices_model;

  auto document = read_musicxml_document(filename);
  static XMLValidator musicxml_validator("musicxml.xsd");
  if (!check_document(window_body, document, musicxml_validator,
                      QObject::tr("Invalid musicxml file"))) {
    return false;
  }

  auto maybe_score = import_score(window_body, get_root(document));
  if (!maybe_score.has_value()) {
    return false;
  }
  auto& score = maybe_score.value();

  reset_switch_table_to_chords(window_body.switch_column);
  clear_rows(chords_model);
  clear_rows(pitched_voices_model);
  clear_rows(unpitched_voices_model);
  add_imported_voices(pitched_voices_model, score.voice_names.pitched);
  add_imported_voices(unpitched_voices_model, score.voice_names.unpitched);

  window_body.controls_column.spin_boxes.starting_key_editor.setValue(
      midi_number_to_frequency(score.starting_midi_key));

  for (auto& chord : score.chords) {
    chords_model.insert_row(chords_model.rowCount(QModelIndex()),
                            std::move(chord));
  }

  finish_loading(window_body);
  return true;
}
