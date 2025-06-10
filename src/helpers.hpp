#pragma once

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QItemSelectionModel>
#include <QtGui/QGuiApplication>
#include <QtGui/QUndoStack>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <fluidsynth.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>

#include "XMLDocument.hpp"
#include "constants.hpp"

static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> auto & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Iterable>
[[nodiscard]] static auto get_only(const Iterable &iterable) -> const auto & {
  Q_ASSERT(iterable.size() == 1);
  return iterable.at(0);
}


template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     const int first_row_number,
                                     const int number_of_rows) {
  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

[[nodiscard]] static auto to_int(const double value) {
  return static_cast<int>(std::round(value));
}

template <typename SubType>
[[nodiscard]] static auto variant_to(const QVariant &variant) {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}


[[nodiscard]] static auto c_string_to_xml_string(const char *text) {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const xmlChar *>(text);
}

[[nodiscard]] static auto xml_string_to_c_string(const xmlChar *text) {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const char *>(text);
}

[[nodiscard]] static auto xml_string_to_string(const xmlChar *text) {
  return std::string(xml_string_to_c_string(text));
}

[[nodiscard]] static inline auto get_xml_name(const xmlNode &node) {
  return xml_string_to_string(node.name);
}

[[nodiscard]] static auto get_c_string_content(const xmlNode &node) {
  return xml_string_to_c_string(xmlNodeGetContent(&node));
}

[[nodiscard]] static auto get_content(const xmlNode &node) {
  return std::string(get_c_string_content(node));
}

[[nodiscard]] static inline auto get_qstring_content(const xmlNode &node) {
  return QString(get_c_string_content(node));
}

[[nodiscard]] static inline auto xml_to_int(const xmlNode &element) {
  return std::stoi(get_content(element));
}

[[nodiscard]] static auto
get_new_child_pointer(xmlNode &node, const char *const field_name,
                      const xmlChar *contents = nullptr) {
  return xmlNewChild(&node, nullptr, c_string_to_xml_string(field_name),
                     contents);
}

[[nodiscard]] static auto inline 
get_new_child(xmlNode &node, const char *const field_name) -> auto & {
  return get_reference(get_new_child_pointer(node, field_name));
}

static void set_xml_string(xmlNode &node, const char *const field_name,
                           const std::string &contents) {
  auto *result = get_new_child_pointer(
      node, field_name, c_string_to_xml_string(contents.c_str()));
  Q_ASSERT(result != nullptr);
}

static void set_xml_int(xmlNode &node, const char *const field_name,
                        int value) {
  set_xml_string(node, field_name, std::to_string(value));
}

static inline void maybe_set_xml_int(xmlNode &node, const char *const field_name,
                                     const int value, const int default_value) {
  if (value != default_value) {
    set_xml_int(node, field_name, value);
  }
}

static inline void maybe_set_xml_qstring(xmlNode &node, const char *const field_name,
                                         const QString &words) {
  if (!words.isEmpty()) {
    set_xml_string(node, field_name, words.toStdString());
  }
}

[[nodiscard]] static auto get_share_file(const char *file_name) {
  static const auto share_folder = []() {
    QDir folder(QCoreApplication::applicationDirPath());
    folder.cdUp();
    folder.cd("share");
    return folder;
  }();
  const auto result_file = share_folder.filePath(file_name);
  Q_ASSERT(QFile::exists(result_file));
  return result_file.toStdString();
}

[[nodiscard]] static inline auto get_soundfont_id(fluid_synth_t &synth) {
  const auto soundfont_id = fluid_synth_sfload(
      &synth, get_share_file("MuseScore_General.sf2").c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

static void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

static inline void set_fluid_int(fluid_settings_t &settings, const char *const field,
                          const int value) {
  Q_ASSERT(field != nullptr);
  check_fluid_ok(fluid_settings_setint(&settings, field, value));
}

[[nodiscard]] static auto inline 
make_audio_driver(QWidget &parent, fluid_settings_t &settings,
    fluid_synth_t &synth) -> fluid_audio_driver_t * {
#ifndef NO_REALTIME_AUDIO
  auto *const audio_driver_pointer = new_fluid_audio_driver(&settings, &synth);
  if (audio_driver_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("Audio driver error"),
                         QObject::tr("Cannot start audio driver"));
  }
  return audio_driver_pointer;
#else
  return nullptr;
#endif
}

static inline void stop_playing(fluid_sequencer_t &sequencer, fluid_event_t &event) {
  fluid_sequencer_remove_events(&sequencer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(&event, channel_number);
    fluid_sequencer_send_now(&sequencer, &event);
  }
}

static inline void
maybe_delete_audio_driver(fluid_audio_driver_t *const audio_driver_pointer) {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }
}

[[nodiscard]] static inline auto midi_number_to_frequency(const double midi_number) {
  return pow(OCTAVE_RATIO,
             (midi_number - CONCERT_A_MIDI) / HALFSTEPS_PER_OCTAVE) *
         CONCERT_A_FREQUENCY;
}

[[nodiscard]] static inline auto frequency_to_midi_number(const double key) {
  Q_ASSERT(key > 0);
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

static inline void send_event_at(fluid_sequencer_t &sequencer, fluid_event_t &event,
                          const double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(fluid_sequencer_send_at(
      &sequencer, &event, static_cast<unsigned int>(std::round(time)), 1));
}

[[nodiscard]] static inline auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

[[nodiscard]] static auto inline 
get_octave_degree(int midi_interval) -> std::tuple<int, int> {
  const int octave =
      to_int(std::floor((1.0 * midi_interval) / HALFSTEPS_PER_OCTAVE));
  return std::make_tuple(octave, midi_interval - octave * HALFSTEPS_PER_OCTAVE);
}


template <std::derived_from<QWidget> SubWidget>
[[nodiscard]] static auto get_minimum_size() -> const auto & {
  static const auto minimum_size = SubWidget(nullptr).minimumSizeHint();
  return minimum_size;
}

static inline void add_menu_action(
    QMenu &menu, QAction &action,
    const QKeySequence::StandardKey key_sequence = QKeySequence::StandardKey(),
    const bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}

static inline void clear_and_clean(QUndoStack &undo_stack) {
  undo_stack.clear();
  undo_stack.setClean();
}

[[nodiscard]] static auto get_root(const XMLDocument &document) -> auto & {
  return get_reference(xmlDocGetRootElement(document.internal_pointer));
}

[[nodiscard]] static inline auto make_root(XMLDocument &document,
                                           const char *field_name) -> auto & {
  auto &root_node =
      get_reference(xmlNewNode(nullptr, c_string_to_xml_string(field_name)));
  xmlDocSetRootElement(document.internal_pointer, &root_node);
  return get_root(document);
}

[[nodiscard]] static inline auto get_selection_model(
    const QAbstractItemView &item_view) -> QItemSelectionModel & {
  return get_reference(item_view.selectionModel());
}

[[nodiscard]] static inline auto get_clipboard() -> auto & {
  return get_reference(QGuiApplication::clipboard());
}

[[nodiscard]] static inline auto make_range(QAbstractItemModel &model,
                                            const int first_row_number,
                                            const int number_of_rows,
                                            const int left_column,
                                            const int right_column) {
  return QItemSelectionRange(
      model.index(first_row_number, left_column),
      model.index(first_row_number + number_of_rows - 1, right_column));
}