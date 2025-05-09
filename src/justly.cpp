#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QIterator>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QStringListModel>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtCore/QtGlobal>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent> // IWYU pragma: keep
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QKeySequence>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>
#include <QtGui/QUndoStack>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QItemEditorFactory>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <fluidsynth.h>
#include <fstream>
#include <iterator>
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlstring.h>
#include <libxml/xmlversion.h>
#include <limits>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <utility>

#include "justly/justly.hpp"

// IWYU pragma: no_include <bits/std_abs.h>

static const auto BEND_PER_HALFSTEP = 4096;
static const auto C_0_MIDI = 12;
static const auto CENTS_PER_HALFSTEP = 100;
static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;
static const auto BREATH_ID = 2;
static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_MIDI = 57;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;
static const auto MAX_RELEASE_TIME = 6000;
static const auto FIFTH_HALFSTEPS = 7;
static const auto FIVE = 5;
static const auto GAIN_STEP = 0.1;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_GAIN = 10;
static const auto MAX_OCTAVE = 9;
static const auto MAX_RATIONAL_DENOMINATOR = 999;
static const auto MAX_RATIONAL_NUMERATOR = 999;
static const auto MAX_STARTING_KEY = 999;
static const auto MAX_STARTING_TEMPO = 999;
static const auto MAX_VELOCITY = 127;
static const auto MAX_MIDI_NUMBER = 127;
static const auto MIDDLE_C_MIDI = 60;
static const auto MILLISECONDS_PER_MINUTE = 60000;
static const auto NUMBER_OF_MIDI_CHANNELS = 64;
static const auto OCTAVE_RATIO = 2.0;
static const auto QUARTER_STEP = 0.5;
static const auto SEVEN = 7;
static const auto START_END_MILLISECONDS = 500;
static const auto TAMBOURINE_MIDI = 54;
static const auto WORDS_WIDTH = 200;
static const auto ZERO_BEND_HALFSTEPS = 2;

enum ChangeId {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id,
  replace_table_id
};

enum Degree {
  c_degree,
  c_sharp_degree,
  d_degree,
  e_flat_degree,
  e_degree,
  f_degree,
  f_sharp_degree,
  g_degree,
  a_flat_degree,
  a_degree,
  b_flat_degree,
  b_degree
};

enum RowType { chord_type, pitched_note_type, unpitched_note_type };

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

[[nodiscard]] static auto to_xml_string(const char *text) {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const xmlChar *>(text);
}

[[nodiscard]] static auto to_c_string(const xmlChar *text) {
  return reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      const char *>(text);
}

[[nodiscard]] static auto to_string(const xmlChar *text) {
  return std::string(to_c_string(text));
}

[[nodiscard]] static auto get_xml_name(const xmlNode &node) {
  return to_string(node.name);
}

[[nodiscard]] static auto node_is(const xmlNode &node, const char *name) {
  return get_xml_name(node) == name;
}

[[nodiscard]] static auto get_first_child_pointer(xmlNode &node,
                                                  const char *name) {
  auto *child_pointer = xmlFirstElementChild(&node);
  while ((child_pointer != nullptr) &&
         !(node_is(get_reference(child_pointer), name))) {
    child_pointer = xmlNextElementSibling(child_pointer);
  }
  return child_pointer;
}

[[nodiscard]] static auto get_child(xmlNode &node, const char *name) -> auto & {
  return get_reference(get_first_child_pointer(node, name));
}

[[nodiscard]] static auto get_c_string_content(const xmlNode &node) {
  return to_c_string(xmlNodeGetContent(&node));
}

[[nodiscard]] static auto get_content(const xmlNode &node) {
  return std::string(get_c_string_content(node));
}

[[nodiscard]] static auto get_qstring_content(const xmlNode &node) {
  return QString(get_c_string_content(node));
}

[[nodiscard]] static auto to_xml_int(const xmlNode &element) {
  return std::stoi(get_content(element));
}

[[nodiscard]] static auto get_xml_int(xmlNode &node,
                                      const char *const field_name) {
  return to_xml_int(get_child(node, field_name));
}

[[nodiscard]] static auto get_xml_int_default(xmlNode &node,
                                              const char *const field_name,
                                              const int default_value) {
  auto *child_pointer = get_first_child_pointer(node, field_name);
  if (child_pointer == nullptr) {
    return default_value;
  }
  return to_xml_int(get_reference(child_pointer));
}

[[nodiscard]] static auto get_number_of_children(xmlNode &node) {
  auto count = 0;
  xmlNode *child_pointer = xmlFirstElementChild(&node);
  while (child_pointer != nullptr) {
    count++;
    child_pointer = xmlNextElementSibling(child_pointer);
  }
  return count;
}

[[nodiscard]] static auto get_xml_double(xmlNode &element,
                                         const char *const field_name) {
  return std::stod(get_content(get_child(element, field_name)));
}

static auto set_xml_c_string(xmlNode &node, const char *const field_name,
                             const xmlChar *contents = nullptr) -> auto & {
  return get_reference(
      xmlNewChild(&node, nullptr, to_xml_string(field_name), contents));
}

[[nodiscard]] static auto
make_empty_child(xmlNode &node, const char *const field_name) -> auto & {
  return set_xml_c_string(node, field_name);
}

static void set_xml_string(xmlNode &node, const char *const field_name,
                           const std::string &contents) {
  set_xml_c_string(node, field_name, to_xml_string(contents.c_str()));
}

static void set_xml_int(xmlNode &node, const char *const field_name,
                        int value) {
  set_xml_string(node, field_name, std::to_string(value));
}

static void set_xml_int_default(xmlNode &node, const char *const field_name,
                                const int value, const int default_value) {
  if (value != default_value) {
    set_xml_int(node, field_name, value);
  }
}

static void set_xml_double(xmlNode &node, const char *const field_name,
                           double value) {
  set_xml_string(node, field_name, std::to_string(value));
}

static void set_xml_qstring(xmlNode &node, const char *const field_name,
                            const QString &words) {
  if (!words.isEmpty()) {
    set_xml_string(node, field_name, words.toStdString());
  }
}

template <std::derived_from<QWidget> SubWidget>
[[nodiscard]] static auto get_minimum_size() -> const auto & {
  static const auto minimum_size = SubWidget(nullptr).minimumSizeHint();
  return minimum_size;
}

[[nodiscard]] static auto get_midi(const double key) {
  Q_ASSERT(key > 0);
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

[[nodiscard]] static auto get_frequency(const double midi_number) {
  return pow(OCTAVE_RATIO,
             (midi_number - CONCERT_A_MIDI) / HALFSTEPS_PER_OCTAVE) *
         CONCERT_A_FREQUENCY;
}

static void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

static void set_fluid_int(fluid_settings_t &settings, const char *const field,
                          const int value) {
  Q_ASSERT(field != nullptr);
  check_fluid_ok(fluid_settings_setint(&settings, field, value));
}

[[nodiscard]] static auto get_share_file_name(const char *file_name) {
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

[[nodiscard]] static auto get_soundfont_id(fluid_synth_t &synth) {
  const auto soundfont_id = fluid_synth_sfload(
      &synth, get_share_file_name("MuseScore_General.sf2").c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

static void send_event_at(fluid_sequencer_t &sequencer, fluid_event_t &event,
                          const double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(fluid_sequencer_send_at(
      &sequencer, &event, static_cast<unsigned int>(std::round(time)), 1));
}

[[nodiscard]] static auto
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

static void
delete_audio_driver(fluid_audio_driver_t *const audio_driver_pointer) {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }
}

static void stop_playing(fluid_sequencer_t &sequencer, fluid_event_t &event) {
  fluid_sequencer_remove_events(&sequencer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(&event, channel_number);
    fluid_sequencer_send_now(&sequencer, &event);
  }
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

static void add_menu_action(
    QMenu &menu, QAction &action,
    const QKeySequence::StandardKey key_sequence = QKeySequence::StandardKey(),
    const bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}

static void clear_and_clean(QUndoStack &undo_stack) {
  undo_stack.clear();
  undo_stack.setClean();
}

static void add_control(QFormLayout &spin_boxes_form, const QString &label,
                        QDoubleSpinBox &spin_box, const int minimum,
                        const int maximum, const QString &suffix,
                        const double single_step = 1, const int decimals = 0) {
  Q_ASSERT(suffix.isValidUtf16());
  Q_ASSERT(label.isValidUtf16());
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSuffix(suffix);
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  spin_boxes_form.addRow(label, &spin_box);
}

struct Program {
  std::string original_name;
  QString translated_name;
  short bank_number;
  short preset_number;

  Program(const char *const original_name_input, const short bank_number_input,
          const short preset_number_input)
      : original_name(original_name_input),
        translated_name(QObject::tr(original_name_input)),
        bank_number(bank_number_input), preset_number(preset_number_input){};
};

template <typename SubProgram> // type properties
concept ProgramInterface = std::derived_from<SubProgram, Program> &&
  requires()
{
  { SubProgram::get_all_programs() } -> std::same_as<const QList<SubProgram> &>;
};

template <ProgramInterface SubProgram>
[[nodiscard]] static auto get_by_original_name(const std::string &name) -> const
    auto & {
  const auto &all_programs = SubProgram::get_all_programs();
  const auto program_pointer = std::find_if(
      all_programs.cbegin(), all_programs.cend(),
      [name](const SubProgram &item) { return item.original_name == name; });
  Q_ASSERT(program_pointer != nullptr);
  return *program_pointer;
}

static void set_xml_program(xmlNode &node, const char *const field_name,
                            const Program *program_pointer) {
  Q_ASSERT(field_name != nullptr);
  if (program_pointer != nullptr) {
    set_xml_string(node, field_name,
                   get_reference(program_pointer).original_name);
  }
}

template <ProgramInterface SubProgram>
[[nodiscard]] static auto
get_xml_program_pointer(xmlNode &node,
                        const char *const field_name) -> const SubProgram * {
  auto *child_pointer = get_first_child_pointer(node, field_name);
  if (child_pointer != nullptr) {
    const auto name = get_content(get_reference(child_pointer));
    const auto &all_programs = SubProgram::get_all_programs();
    const auto program_pointer = std::find_if(
        all_programs.cbegin(), all_programs.cend(),
        [name](const SubProgram &item) { return item.original_name == name; });
    Q_ASSERT(program_pointer != nullptr);
    return &(*program_pointer);
  };
  return nullptr;
}

template <std::derived_from<Program> SubProgram>
[[nodiscard]] static auto get_programs(const bool is_percussion) {
  auto &settings = get_reference(new_fluid_settings());
  auto &synth = get_reference(new_fluid_synth(&settings));

  fluid_sfont_t *const soundfont_pointer =
      fluid_synth_get_sfont_by_id(&synth, get_soundfont_id(synth));
  Q_ASSERT(soundfont_pointer != nullptr);

  fluid_sfont_iteration_start(soundfont_pointer);
  auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  static const std::set<std::string> skip_names(
      {// dummy programs
       "Basses Fast Expr.", "Basses Pizzicato Expr.", "Basses Slow Expr.",
       "Basses Trem Expr.", "Celli Fast Expr.", "Celli Pizzicato Expr.",
       "Celli Slow Expr.", "Celli Trem Expr.", "Violas Fast Expr.",
       "Violas Pizzicato Expr.", "Violas Slow Expr.", "Violas Trem Expr.",
       "Violins Fast Expr.", "Violins Pizzicato Expr.", "Violins Slow Expr.",
       "Violins Trem Expr.", "Violins2 Fast Expr.", "Violins2 Pizzicato Expr.",
       "Violins2 Slow Expr.", "Violins2 Trem Expr.",
       // non-expressive programs
       "5th Saw Wave", "Accordion", "Alto Sax", "Atmosphere", "Bandoneon",
       "Baritone Sax", "Bass & Lead", "Basses Fast", "Basses Fast",
       "Basses Pizzicato", "Basses Slow", "Basses Slow", "Basses Trem",
       "Basses Tremolo", "Bassoon", "Bottle Chiff", "Bowed Glass", "Brass 2",
       "Brass Section", "Calliope Lead", "Celli Fast", "Celli Fast",
       "Celli Pizzicato", "Celli Slow", "Celli Slow", "Celli Trem",
       "Celli Tremolo", "Cello", "Charang", "Chiffer Lead", "Choir Aahs",
       "Church Organ 2", "Church Organ", "Clarinet", "Contrabass",
       "Detuned Organ 1", "Detuned Organ 2", "Detuned Saw", "Drawbar Organ",
       "Echo Drops", "English Horn", "Fiddle", "Flute", "French Horns",
       "Goblin", "Halo Pad", "Harmon Mute Trumpet", "Harmonica",
       "Italian Accordion", "Metal Pad", "Oboe", "Ocarina", "Pan Flute",
       "Percussive Organ", "Piccolo", "Polysynth", "Recorder", "Reed Organ",
       "Rock Organ", "Saw Lead", "Shakuhachi", "Shenai", "Sine Wave",
       "Slow Violin", "Solo Vox", "Soprano Sax", "Soundtrack", "Space Voice",
       "Square Lead", "Star Theme", "Strings Fast", "Strings Slow",
       "Strings Tremolo", "Sweep Pad", "Synth Brass 1", "Synth Brass 2",
       "Synth Brass 3", "Synth Brass 4", "Synth Strings 1", "Synth Strings 2",
       "Synth Strings 3", "Synth Voice", "Tenor Sax", "Trombone", "Trumpet",
       "Tuba", "Viola", "Violas Fast", "Violas Fast", "Violas Pizzicato",
       "Violas Slow", "Violas Slow", "Violas Trem", "Violas Tremolo", "Violin",
       "Violins Fast", "Violins Fast", "Violins Pizzicato", "Violins Slow",
       "Violins Slow", "Violins Trem", "Violins Tremolo", "Violins2 Fast",
       "Violins2 Fast", "Violins2 Pizzicato", "Violins2 Slow", "Violins2 Slow",
       "Violins2 Trem", "Violins2 Tremolo", "Voice Oohs", "Warm Pad", "Whistle",
       // not working?
       "Temple Blocks"});

  static const std::set<std::string> percussion_set_names({"Brush 1",
                                                           "Brush 2",
                                                           "Brush",
                                                           "Electronic",
                                                           "Jazz 1",
                                                           "Jazz 2",
                                                           "Jazz 3",
                                                           "Jazz 4",
                                                           "Jazz",
                                                           "Marching Bass",
                                                           "Marching Cymbals",
                                                           "Marching Snare",
                                                           "Marching Tenor",
                                                           "OldMarchingBass",
                                                           "OldMarchingTenor",
                                                           "Orchestra Kit",
                                                           "Power 1",
                                                           "Power 2",
                                                           "Power 3",
                                                           "Power",
                                                           "Room 1",
                                                           "Room 2",
                                                           "Room 3",
                                                           "Room 4",
                                                           "Room 5",
                                                           "Room 6",
                                                           "Room 7",
                                                           "Room",
                                                           "Standard 1",
                                                           "Standard 2",
                                                           "Standard 3",
                                                           "Standard 4",
                                                           "Standard 5",
                                                           "Standard 6",
                                                           "Standard 7",
                                                           "Standard",
                                                           "TR-808"});

  QList<SubProgram> programs;
  while (preset_pointer != nullptr) {
    const auto *const name = fluid_preset_get_name(preset_pointer);
    if (!skip_names.contains(name) &&
        is_percussion == percussion_set_names.contains(name)) {
      programs.push_back(SubProgram(
          name, static_cast<short>(fluid_preset_get_banknum(preset_pointer)),
          static_cast<short>(fluid_preset_get_num(preset_pointer))));
    }
    preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  }

  delete_fluid_synth(&synth);
  delete_fluid_settings(&settings);

  std::sort(programs.begin(), programs.end(),
            [](const SubProgram &instrument_1, const SubProgram &instrument_2) {
              return instrument_1.translated_name <=
                     instrument_2.translated_name;
            });

  return programs;
}

struct PercussionSet : public Program {
  PercussionSet(const char *const name, const short bank_number,
                const short preset_number)
      : Program(name, bank_number, preset_number){};

  [[nodiscard]] static auto get_all_programs() -> const QList<PercussionSet> & {
    static const auto all_percussion_sets = get_programs<PercussionSet>(true);
    return all_percussion_sets;
  }
};

Q_DECLARE_METATYPE(const PercussionSet *);

struct PercussionInstrument {
  const PercussionSet *percussion_set_pointer;
  short midi_number;

  explicit PercussionInstrument(
      const PercussionSet *const percussion_set_pointer_input = nullptr,
      const short midi_number_input = TAMBOURINE_MIDI)
      : percussion_set_pointer(percussion_set_pointer_input),
        midi_number(midi_number_input){};

  [[nodiscard]] auto
  operator==(const PercussionInstrument &other_percussion_instrument) const {
    return percussion_set_pointer ==
               other_percussion_instrument.percussion_set_pointer &&
           midi_number == other_percussion_instrument.midi_number;
  }
};

Q_DECLARE_METATYPE(PercussionInstrument);

[[nodiscard]] static auto percussion_instrument_is_default(
    const PercussionInstrument &percussion_instrument) {
  return percussion_instrument.percussion_set_pointer == nullptr;
}

struct Instrument : public Program {
  Instrument(const char *const name, const short bank_number,
             const short preset_number)
      : Program(name, bank_number, preset_number){};

  [[nodiscard]] static auto get_all_programs() -> const QList<Instrument> & {
    static const auto all_instruments = get_programs<Instrument>(false);
    return all_instruments;
  }
};

Q_DECLARE_METATYPE(const Instrument *);

struct Rational {
  int numerator;
  int denominator;

  explicit Rational(const int numerator_input = 1,
                    const int denominator_input = 1) {
    Q_ASSERT(denominator_input != 0);
    const auto common_denominator =
        std::gcd(numerator_input, denominator_input);
    numerator = numerator_input / common_denominator;
    denominator = denominator_input / common_denominator;
  }

  [[nodiscard]] auto operator*(const Rational &other_interval) const {
    return Rational(numerator * other_interval.numerator,
                    denominator * other_interval.denominator);
  }

  [[nodiscard]] auto operator/(const Rational &other_interval) const {
    return Rational(numerator * other_interval.denominator,
                    denominator * other_interval.numerator);
  }

  [[nodiscard]] auto operator==(const Rational &other_rational) const {
    return numerator == other_rational.numerator &&
           denominator == other_rational.denominator;
  }
};

Q_DECLARE_METATYPE(Rational);

[[nodiscard]] static auto rational_to_double(const Rational &rational) {
  const auto denominator = rational.denominator;
  Q_ASSERT(denominator != 0);
  return (1.0 * rational.numerator) / denominator;
}

[[nodiscard]] static auto rational_is_default(const Rational &rational) {
  return rational.numerator == 1 && rational.denominator == 1;
}

[[nodiscard]] static auto get_xml_rational(xmlNode &node,
                                           const char *const field_name) {
  auto *child_node_pointer = get_first_child_pointer(node, field_name);
  if (child_node_pointer != nullptr) {
    auto &rational_node = get_reference(child_node_pointer);
    return Rational(get_xml_int_default(rational_node, "numerator", 1),
                    get_xml_int_default(rational_node, "denominator", 1));
  }
  return Rational();
}

static void set_xml_rational(xmlNode &node, const Rational &rational,
                             const char *const column_name) {
  if (!rational_is_default(rational)) {
    auto &rational_node = make_empty_child(node, column_name);
    set_xml_int_default(rational_node, "numerator", rational.numerator, 1);
    set_xml_int_default(rational_node, "denominator", rational.denominator, 1);
  }
}

struct Interval {
  Rational ratio;
  int octave;

  explicit Interval(Rational ratio_input = Rational(1, 1),
                    const int octave_input = 0)
      : ratio(ratio_input), octave(octave_input) {
    while (ratio.numerator % 2 == 0) {
      ratio.numerator = ratio.numerator / 2;
      octave = octave + 1;
    }
    while (ratio.denominator % 2 == 0) {
      ratio.denominator = ratio.denominator / 2;
      octave = octave - 1;
    }
  }

  [[nodiscard]] auto operator==(const Interval &other_interval) const {
    return ratio == other_interval.ratio && octave == other_interval.octave;
  }

  [[nodiscard]] auto operator*(const Interval &other_interval) const {
    return Interval(ratio * other_interval.ratio,
                    octave + other_interval.octave);
  }

  [[nodiscard]] auto operator/(const Interval &other_interval) const {
    return Interval(ratio / other_interval.ratio,
                    octave - other_interval.octave);
  }
};

Q_DECLARE_METATYPE(Interval);

[[nodiscard]] static auto interval_to_double(const Interval &interval) {
  return rational_to_double(interval.ratio) *
         pow(OCTAVE_RATIO, interval.octave);
}

[[nodiscard]] static auto get_xml_interval(xmlNode &node,
                                           const char *const field_name) {
  auto *child_pointer = get_first_child_pointer(node, field_name);
  if (child_pointer != nullptr) {
    auto &interval_node = get_reference(child_pointer);
    return Interval(get_xml_rational(interval_node, "ratio"),
                    get_xml_int_default(interval_node, "octave", 0));
  }
  return Interval();
}

static void set_xml_interval(xmlNode &node, const Interval &interval,
                             const char *const column_name) {
  const auto &ratio = interval.ratio;
  const auto octave = interval.octave;
  if (!rational_is_default(ratio) || octave != 0) {
    auto &interval_node = make_empty_child(node, column_name);
    set_xml_rational(interval_node, ratio, "ratio");
    set_xml_int_default(interval_node, "octave", octave, 0);
  }
}

template <ProgramInterface SubProgram> struct ProgramEditor : public QComboBox {
  explicit ProgramEditor(QWidget *const parent_pointer)
      : QComboBox(parent_pointer) {
    static auto names_model = []() {
      const auto &all_programs = SubProgram::get_all_programs();
      QList<QString> names({""});
      std::transform(
          all_programs.cbegin(), all_programs.cend(), std::back_inserter(names),
          [](const SubProgram &item) { return item.translated_name; });
      return QStringListModel(names);
    }();
    setModel(&names_model);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  }

  [[nodiscard]] auto value() const -> const SubProgram * {
    const auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &SubProgram::get_all_programs().at(row - 1);
  }

  void setValue(const SubProgram *new_value) {
    setCurrentIndex(
        new_value == nullptr
            ? 0
            : static_cast<int>(std::distance(
                  SubProgram::get_all_programs().data(), new_value)) +
                  1);
  }
};

struct InstrumentEditor : public ProgramEditor<Instrument> {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *const parent_pointer)
      : ProgramEditor<Instrument>(parent_pointer) {}
};

struct PercussionSetEditor : public ProgramEditor<PercussionSet> {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *const parent_pointer_input)
      : ProgramEditor<PercussionSet>(parent_pointer_input) {}
};

struct PercussionInstrumentEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(PercussionInstrument value READ value WRITE setValue USER true)

  ProgramEditor<PercussionSet> &percussion_set_editor =
      *(new ProgramEditor<PercussionSet>(this));
  QLabel &number_text = *(new QLabel("#"));
  QSpinBox &midi_number_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

public:
  explicit PercussionInstrumentEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    midi_number_box.setMinimum(0);
    midi_number_box.setMaximum(MAX_MIDI_NUMBER);

    row_layout.addWidget(&percussion_set_editor);
    row_layout.addWidget(&number_text);
    row_layout.addWidget(&midi_number_box);
    row_layout.setContentsMargins(0, 0, 0, 0);
  }
  [[nodiscard]] auto value() const -> PercussionInstrument {
    return PercussionInstrument(percussion_set_editor.value(),
                                static_cast<short>(midi_number_box.value()));
  }
  void setValue(const PercussionInstrument &new_value) const {
    percussion_set_editor.setValue(new_value.percussion_set_pointer);
    midi_number_box.setValue(new_value.midi_number);
  }
};

struct RationalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  QSpinBox &numerator_box = *(new QSpinBox);
  QLabel &slash_text = *(new QLabel("/"));
  QSpinBox &denominator_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit RationalEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    numerator_box.setMinimum(1);
    numerator_box.setMaximum(MAX_RATIONAL_NUMERATOR);

    denominator_box.setMinimum(1);
    denominator_box.setMaximum(MAX_RATIONAL_DENOMINATOR);

    row_layout.addWidget(&numerator_box);
    row_layout.addWidget(&slash_text);
    row_layout.addWidget(&denominator_box);
    row_layout.setContentsMargins(1, 0, 1, 0);
  }

  [[nodiscard]] auto value() const {
    return Rational(numerator_box.value(), denominator_box.value());
  }

  void setValue(const Rational &new_value) const {
    numerator_box.setValue(new_value.numerator);
    denominator_box.setValue(new_value.denominator);
  }
};

struct IntervalEditor : QFrame {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

public:
  RationalEditor &rational_editor;

  QWidget &o_text = *(new QLabel("o"));
  QSpinBox &octave_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit IntervalEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer),
        rational_editor(*(new RationalEditor(parent_pointer))) {

    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    rational_editor.setFrameShape(QFrame::NoFrame);

    octave_box.setMinimum(-MAX_OCTAVE);
    octave_box.setMaximum(MAX_OCTAVE);

    row_layout.addWidget(&rational_editor);
    row_layout.addWidget(&o_text);
    row_layout.addWidget(&octave_box);
    row_layout.setContentsMargins(1, 0, 1, 0);
  }

  [[nodiscard]] auto value() const {
    return Interval(rational_editor.value(), octave_box.value());
  }

  void setValue(const Interval &new_value) const {
    rational_editor.setValue(new_value.ratio);
    octave_box.setValue(new_value.octave);
  }
};

void set_up() {
  LIBXML_TEST_VERSION

  QApplication::setApplicationDisplayName("Justly");

  const QPixmap pixmap(get_share_file_name("Justly.svg").c_str());
  if (!pixmap.isNull()) {
    QApplication::setWindowIcon(QIcon(pixmap));
  }

  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) {
    const auto numerator = rational.numerator;
    const auto denominator = rational.denominator;

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
    const auto numerator = interval.ratio.numerator;
    const auto denominator = interval.ratio.denominator;
    const auto octave = interval.octave;

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
      [](const Instrument *instrment_pointer) {
        if (instrment_pointer == nullptr) {
          return QString("");
        }
        return get_reference(instrment_pointer).translated_name;
      });
  QMetaType::registerConverter<PercussionInstrument, QString>(
      [](const PercussionInstrument &percussion_instrument) {
        if (percussion_instrument_is_default(percussion_instrument)) {
          return QString("");
        }
        QString result;
        QTextStream stream(&result);
        stream << get_reference(percussion_instrument.percussion_set_pointer)
                      .translated_name
               << " #" << percussion_instrument.midi_number;
        return result;
      });

  auto &factory = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QItemEditorFactory);
  factory.registerEditor(
      qMetaTypeId<Rational>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          RationalEditor>);
  factory.registerEditor(
      qMetaTypeId<PercussionInstrument>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionInstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<const Instrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          InstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<Interval>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          IntervalEditor>);
  factory.registerEditor(
      qMetaTypeId<QString>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QLineEdit>);
  QItemEditorFactory::setDefaultFactory(&factory);
}

struct Row {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  Row() = default;

  explicit Row(xmlNode &node)
      : beats(get_xml_rational(node, "beats")),
        velocity_ratio(get_xml_rational(node, "velocity_ratio")),
        words([](xmlNode *words_node_pointer) {
          if (words_node_pointer != nullptr) {
            return get_qstring_content(get_reference(words_node_pointer));
          }
          return QString("");
        }(get_first_child_pointer(node, "words"))) {}

  virtual ~Row() = default;

  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;

  virtual void set_data(int column, const QVariant &new_value) = 0;
  virtual void column_to_xml(xmlNode &node, int column_number) const = 0;
};

template <typename SubRow>
concept RowInterface =
    std::derived_from<SubRow, Row> &&
    requires(SubRow target_row, const SubRow &template_row, xmlNode &node,
             int column_number) {
      { SubRow(node) } -> std::same_as<SubRow>;
      {
        target_row.copy_column_from(template_row, column_number)
      } -> std::same_as<void>;
      { SubRow::get_number_of_columns() } -> std::same_as<int>;
      { SubRow::get_column_name(column_number) } -> std::same_as<const char *>;
      { SubRow::get_cells_mime() } -> std::same_as<const char *>;
      { SubRow::is_column_editable(column_number) } -> std::same_as<bool>;
    };

template <RowInterface SubRow>
static void partial_xml_to_rows(QList<SubRow> &new_rows, xmlNode &node,
                                const int number_of_rows) {
  auto *child_pointer = xmlFirstElementChild(&node);
  for (auto index = 0; index < number_of_rows; index++) {
    new_rows.push_back(SubRow(get_reference(child_pointer)));
    child_pointer = xmlNextElementSibling(child_pointer);
  }
}

template <RowInterface SubRow>
static void xml_to_rows(QList<SubRow> &rows, xmlNode &node) {
  partial_xml_to_rows(rows, node, get_number_of_children(node));
}

template <RowInterface SubRow>
static void partial_set_xml_rows(xmlNode &node, const char *const array_name,
                                 const QList<SubRow> &rows, const int first_row,
                                 const int number_of_rows,
                                 const int left_column,
                                 const int right_column) {
  if (!rows.empty()) {
    auto &rows_node = make_empty_child(node, array_name);
    for (int index = first_row; index < first_row + number_of_rows; index++) {
      auto &row = rows[index];
      auto &row_node = make_empty_child(rows_node, SubRow::get_xml_name());
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.column_to_xml(row_node, column_number);
      }
    }
  }
}

template <RowInterface SubRow>
static void set_xml_rows(xmlNode &node, const char *const array_name,
                         const QList<SubRow> &rows) {
  partial_set_xml_rows(node, array_name, rows, 0, rows.size(), 0,
                       SubRow::get_number_of_columns() - 1);
}

template <RowInterface SubRow>
[[nodiscard]] static auto get_xml_rows(xmlNode &node,
                                       const char *const field_name) {
  auto *rows_node_pointer = get_first_child_pointer(node, field_name);
  if (rows_node_pointer != nullptr) {
    QList<SubRow> rows;
    xml_to_rows(rows, get_reference(rows_node_pointer));
    return rows;
  }
  return QList<SubRow>();
}

[[nodiscard]] static auto get_milliseconds(const double beats_per_minute,
                                           const Rational &beats) {
  return rational_to_double(beats) * MILLISECONDS_PER_MINUTE / beats_per_minute;
}

struct PlayState {
  double current_time = 0;

  const Instrument *current_instrument_pointer = nullptr;
  PercussionInstrument current_percussion_instrument;
  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;
};

struct Note : Row {
  Note() = default;
  explicit Note(xmlNode &node) : Row(node){};

  [[nodiscard]] virtual auto
  get_closest_midi(QWidget &parent, fluid_sequencer_t &sequencer,
                   fluid_event_t &event, const PlayState &play_state,
                   int channel_number, int chord_number,
                   int note_number) const -> std::optional<short> = 0;

  [[nodiscard]] virtual auto
  get_program_pointer(QWidget &parent, const PlayState &play_state,
                      int chord_number,
                      int note_number) const -> const Program * = 0;
};

template <typename SubNote> // type properties
concept NoteInterface = std::derived_from<SubNote, Note> &&
  requires()
{
  { SubNote::get_description() } -> std::same_as<const char *>;
};

template <NoteInterface SubNote>
static void add_note_location(QTextStream &stream, const int chord_number,
                              const int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1
         << QObject::tr(SubNote::get_description()) << note_number + 1;
}

void set_xml_percussion_instrument(
    xmlNode &node, const char *const field_name,
    const PercussionInstrument &percussion_instrument) {
  if (!(percussion_instrument_is_default(percussion_instrument))) {
    auto &percussion_instrument_node = make_empty_child(node, field_name);
    set_xml_string(percussion_instrument_node, "percussion_set",
                   get_reference(percussion_instrument.percussion_set_pointer)
                       .original_name);
    set_xml_int(percussion_instrument_node, "midi_number",
                percussion_instrument.midi_number);
  }
}

static auto get_xml_percussion_instrument(xmlNode &node,
                                          const char *const field_name) {
  auto *percussion_node_pointer = get_first_child_pointer(node, field_name);
  if (percussion_node_pointer != nullptr) {
    auto &xml_percussion_instrument = get_reference(percussion_node_pointer);
    return PercussionInstrument(
        get_xml_program_pointer<PercussionSet>(xml_percussion_instrument,
                                               "percussion_set"),
        static_cast<short>(
            get_xml_int(xml_percussion_instrument, "midi_number")));
  };
  return PercussionInstrument();
}

struct UnpitchedNote : Note {
  PercussionInstrument percussion_instrument;

  UnpitchedNote() = default;

  explicit UnpitchedNote(xmlNode &node)
      : Note(node), percussion_instrument(get_xml_percussion_instrument(
                        node, "percussion_instrument")) {}

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "unpitched_notes_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_name() -> const char * {
    return "unpitched_note";
  };

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_unpitched_note_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case unpitched_note_percussion_instrument_column:
      return "Percussion instrument";
    case unpitched_note_beats_column:
      return "Beats";
    case unpitched_note_velocity_ratio_column:
      return "Velocity ratio";
    case unpitched_note_words_column:
      return "Words";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.unpitched_notes_cells+xml";
  }

  [[nodiscard]] static auto get_description() { return ", unpitched note "; }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] static auto is_pitched() { return false; }

  [[nodiscard]] auto
  get_closest_midi(QWidget & /*parent*/, fluid_sequencer_t & /*sequencer*/,
                   fluid_event_t & /*event*/, const PlayState &play_state,
                   const int /*channel_number*/, int /*chord_number*/,
                   int /*note_number*/) const -> std::optional<short> override {
    if (percussion_instrument_is_default(percussion_instrument)) {
      return play_state.current_percussion_instrument.midi_number;
    }
    return percussion_instrument.midi_number;
  };

  [[nodiscard]] auto
  get_program_pointer(QWidget &parent, const PlayState &play_state,
                      const int chord_number,
                      const int note_number) const -> const Program * override {
    if (percussion_instrument_is_default(percussion_instrument)) {
      const auto *current_percussion_set_pointer =
          play_state.current_percussion_instrument.percussion_set_pointer;
      if (current_percussion_set_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << QObject::tr("No percussion set");
        add_note_location<UnpitchedNote>(stream, chord_number, note_number);
        QMessageBox::warning(&parent, QObject::tr("Percussion set error"),
                             message);
      }
      return current_percussion_set_pointer;
    }
    return percussion_instrument.percussion_set_pointer;
  };

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case unpitched_note_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument);
    case unpitched_note_beats_column:
      return QVariant::fromValue(beats);
    case unpitched_note_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case unpitched_note_words_column:
      return words;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case unpitched_note_percussion_instrument_column:
      percussion_instrument = variant_to<PercussionInstrument>(new_value);
      break;
    case unpitched_note_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case unpitched_note_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case unpitched_note_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const UnpitchedNote &template_row,
                        const int column_number) {
    switch (column_number) {
    case unpitched_note_percussion_instrument_column:
      percussion_instrument = template_row.percussion_instrument;
      break;
    case unpitched_note_beats_column:
      beats = template_row.beats;
      break;
    case unpitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case unpitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &node, const int column_number) const override {
    switch (column_number) {
    case unpitched_note_percussion_instrument_column:
      set_xml_percussion_instrument(node, "percussion_instrument",
                                    percussion_instrument);
      break;
    case unpitched_note_beats_column:
      set_xml_rational(node, beats, "beats");
      break;
    case unpitched_note_velocity_ratio_column:
      set_xml_rational(node, velocity_ratio, "velocity_ratio");
      break;
    case unpitched_note_words_column:
      set_xml_qstring(node, "words", words);
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

struct PitchedNote : Note {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;

  PitchedNote() = default;

  explicit PitchedNote(xmlNode &node)
      : Note(node), instrument_pointer(get_xml_program_pointer<Instrument>(
                        node, "instrument")),
        interval(get_xml_interval(node, "interval")) {}

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "pitched_notes_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_name() -> const char * {
    return "pitched_note";
  };

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_pitched_note_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case pitched_note_instrument_column:
      return "Instrument";
    case pitched_note_interval_column:
      return "Interval";
    case pitched_note_beats_column:
      return "Beats";
    case pitched_note_velocity_ratio_column:
      return "Velocity ratio";
    case pitched_note_words_column:
      return "Words";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.pitched_notes_cells+xml";
  }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] static auto get_description() { return ", pitched note "; }

  [[nodiscard]] static auto is_pitched() { return true; }

  [[nodiscard]] auto get_closest_midi(
      QWidget &parent, fluid_sequencer_t &sequencer, fluid_event_t &event,
      const PlayState &play_state, const int channel_number,
      const int chord_number,
      const int note_number) const -> std::optional<short> override {
    const auto frequency =
        play_state.current_key * interval_to_double(interval);
    static const auto minimum_frequency = get_frequency(0 - QUARTER_STEP);
    if (frequency < minimum_frequency) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Frequency ") << frequency;
      add_note_location<PitchedNote>(stream, chord_number, note_number);
      stream << QObject::tr(" less than minimum frequency ")
             << minimum_frequency;
      QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
      return {};
    }

    static const auto maximum_frequency =
        get_frequency(MAX_MIDI_NUMBER + QUARTER_STEP);
    if (frequency >= maximum_frequency) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Frequency ") << frequency;
      add_note_location<PitchedNote>(stream, chord_number, note_number);
      stream << QObject::tr(" greater than or equal to maximum frequency ")
             << maximum_frequency;
      QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
      return {};
    }

    const auto midi_float = get_midi(frequency);
    const auto closest_midi = static_cast<short>(round(midi_float));
    fluid_event_pitch_bend(
        &event, channel_number,
        to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
               BEND_PER_HALFSTEP));
    send_event_at(sequencer, event, play_state.current_time);
    return closest_midi;
  }

  [[nodiscard]] auto
  get_program_pointer(QWidget &parent, const PlayState &play_state,
                      const int chord_number,
                      const int note_number) const -> const Program * override {
    if (instrument_pointer == nullptr) {
      const auto *current_instrument_pointer =
          play_state.current_instrument_pointer;
      if (current_instrument_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << QObject::tr("No instrument");
        add_note_location<PitchedNote>(stream, chord_number, note_number);
        QMessageBox::warning(&parent, QObject::tr("Instrument error"), message);
      }
      return current_instrument_pointer;
    }
    return instrument_pointer;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case pitched_note_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case pitched_note_interval_column:
      return QVariant::fromValue(interval);
    case pitched_note_beats_column:
      return QVariant::fromValue(beats);
    case pitched_note_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case pitched_note_words_column:
      return words;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case pitched_note_instrument_column:
      instrument_pointer = variant_to<const Instrument *>(new_value);
      break;
    case pitched_note_interval_column:
      interval = variant_to<Interval>(new_value);
      break;
    case pitched_note_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case pitched_note_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const PitchedNote &template_row,
                        const int column_number) {
    switch (column_number) {
    case pitched_note_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case pitched_note_interval_column:
      interval = template_row.interval;
      break;
    case pitched_note_beats_column:
      beats = template_row.beats;
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case pitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &node, const int column_number) const override {
    switch (column_number) {
    case pitched_note_instrument_column:
      set_xml_program(node, "instrument", instrument_pointer);
      break;
    case pitched_note_interval_column:
      set_xml_interval(node, interval, "interval");
      break;
    case pitched_note_beats_column:
      set_xml_rational(node, beats, "beats");
      break;
    case pitched_note_velocity_ratio_column:
      set_xml_rational(node, velocity_ratio, "velocity_ratio");
      break;
    case pitched_note_words_column:
      set_xml_qstring(node, "words", words);
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }
};

struct Chord : public Row {
  const Instrument *instrument_pointer = nullptr;
  PercussionInstrument percussion_instrument;
  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  Chord() = default;

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "chords_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_name() -> const char * { return "chord"; };

  explicit Chord(xmlNode &chord_node)
      : Row(chord_node), instrument_pointer(get_xml_program_pointer<Instrument>(
                             chord_node, "instrument")),
        percussion_instrument(
            get_xml_percussion_instrument(chord_node, "percussion_instrument")),
        interval(get_xml_interval(chord_node, "interval")),
        tempo_ratio(get_xml_rational(chord_node, "tempo_ratio")),
        pitched_notes(get_xml_rows<PitchedNote>(chord_node, "pitched_notes")),
        unpitched_notes(
            get_xml_rows<UnpitchedNote>(chord_node, "unpitched_notes")) {}

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_chord_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      return "Instrument";
    case chord_percussion_instrument_column:
      return "Percussion instrument";
    case chord_interval_column:
      return "Interval";
    case chord_beats_column:
      return "Beats";
    case chord_velocity_ratio_column:
      return "Velocity ratio";
    case chord_tempo_ratio_column:
      return "Tempo ratio";
    case chord_words_column:
      return "Words";
    case chord_pitched_notes_column:
      return "Pitched notes";
    case chord_unpitched_notes_column:
      return "Unpitched notes";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.chords_cells+xml";
  }

  [[nodiscard]] static auto is_column_editable(int column_number) -> bool {
    return column_number != chord_pitched_notes_column &&
           column_number != chord_unpitched_notes_column;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case chord_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case chord_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument);
    case chord_interval_column:
      return QVariant::fromValue(interval);
    case chord_beats_column:
      return QVariant::fromValue(beats);
    case chord_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case chord_tempo_ratio_column:
      return QVariant::fromValue(tempo_ratio);
    case chord_words_column:
      return words;
    case chord_pitched_notes_column:
      return pitched_notes.size();
    case chord_unpitched_notes_column:
      return unpitched_notes.size();
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = variant_to<const Instrument *>(new_value);
      break;
    case chord_percussion_instrument_column:
      percussion_instrument = variant_to<PercussionInstrument>(new_value);
      break;
    case chord_interval_column:
      interval = variant_to<Interval>(new_value);
      break;
    case chord_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = variant_to<Rational>(new_value);
      break;
    case chord_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const Chord &template_row, const int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case chord_percussion_instrument_column:
      percussion_instrument = template_row.percussion_instrument;
      break;
    case chord_interval_column:
      interval = template_row.interval;
      break;
    case chord_beats_column:
      beats = template_row.beats;
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = template_row.tempo_ratio;
      break;
    case chord_words_column:
      words = template_row.words;
      break;
    case chord_pitched_notes_column:
      pitched_notes = template_row.pitched_notes;
      break;
    case chord_unpitched_notes_column:
      unpitched_notes = template_row.unpitched_notes;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &chord_node,
                     const int column_number) const override {
    switch (column_number) {
    case chord_instrument_column:
      set_xml_program(chord_node, "instrument", instrument_pointer);
      break;
    case chord_percussion_instrument_column:
      set_xml_percussion_instrument(chord_node, "percussion_instrument",
                                    percussion_instrument);
      break;
    case chord_interval_column:
      set_xml_interval(chord_node, interval, "interval");
      break;
    case chord_beats_column:
      set_xml_rational(chord_node, beats, "beats");
      break;
    case chord_velocity_ratio_column:
      set_xml_rational(chord_node, velocity_ratio, "velocity_ratio");
      break;
    case chord_tempo_ratio_column:
      set_xml_rational(chord_node, tempo_ratio, "tempo_ratio");
      break;
    case chord_words_column:
      set_xml_qstring(chord_node, "words", words);
      break;
    case chord_pitched_notes_column:
      set_xml_rows(chord_node, "pitched_notes", pitched_notes);
      break;
    case chord_unpitched_notes_column:
      set_xml_rows(chord_node, "unpitched_notes", unpitched_notes);
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

struct Song {
  double starting_key = get_frequency(DEFAULT_STARTING_MIDI);
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
};

static void set_double(Song &song, fluid_synth_t &synth,
                       const ChangeId control_id, QDoubleSpinBox &spin_box,
                       const double set_value) {
  switch (control_id) {
  case gain_id:
    fluid_synth_set_gain(&synth, static_cast<float>(set_value));
    break;
  case starting_key_id:
    song.starting_key = set_value;
    break;
  case starting_velocity_id:
    song.starting_velocity = set_value;
    break;
  case starting_tempo_id:
    song.starting_tempo = set_value;
    break;
  default:
    Q_ASSERT(false);
    break;
  }
  const QSignalBlocker blocker(spin_box);
  spin_box.setValue(set_value);
}

[[nodiscard]] auto
get_octave_degree(int midi_interval) -> std::tuple<int, int> {
  const int octave =
      to_int(std::floor((1.0 * midi_interval) / HALFSTEPS_PER_OCTAVE));
  return std::make_tuple(octave, midi_interval - octave * HALFSTEPS_PER_OCTAVE);
}

static void initialize_playstate(const Song &song, PlayState &play_state,
                                 double current_time) {
  play_state.current_instrument_pointer = nullptr;
  play_state.current_percussion_instrument = PercussionInstrument(nullptr, 0);
  play_state.current_key = song.starting_key;
  play_state.current_velocity = song.starting_velocity;
  play_state.current_tempo = song.starting_tempo;
  play_state.current_time = current_time;
}

static void modulate(PlayState &play_state, const Chord &chord) {
  play_state.current_key =
      play_state.current_key * interval_to_double(chord.interval);
  play_state.current_velocity =
      play_state.current_velocity * rational_to_double(chord.velocity_ratio);
  play_state.current_tempo =
      play_state.current_tempo * rational_to_double(chord.tempo_ratio);
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    play_state.current_instrument_pointer = chord_instrument_pointer;
  }

  const auto &chord_percussion_instrument = chord.percussion_instrument;
  if (chord_percussion_instrument.percussion_set_pointer != nullptr) {
    play_state.current_percussion_instrument = chord_percussion_instrument;
  }
}

static void move_time(PlayState &play_state, const Chord &chord) {
  play_state.current_time =
      play_state.current_time +
      get_milliseconds(play_state.current_tempo, chord.beats);
}

[[nodiscard]] static auto get_status_text(const Song &song,
                                          const int chord_number,
                                          const double key_ratio = 1,
                                          const double velocity_ratio = 1) {
  PlayState play_state;
  initialize_playstate(song, play_state, 0);
  const auto &chords = song.chords;
  for (auto previous_chord_number = 0; previous_chord_number < chord_number;
       previous_chord_number++) {
    const auto &chord = chords.at(previous_chord_number);
    modulate(play_state, chord);
    move_time(play_state, chord);
  }

  modulate(play_state, chords.at(chord_number));

  const auto key = play_state.current_key * key_ratio;
  const auto midi_float = get_midi(key);
  const auto closest_midi = to_int(midi_float);
  const auto [octave, degree] = get_octave_degree(closest_midi - C_0_MIDI);
  const auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  static const QMap<int, QString> degrees_to_name{
      {0, QObject::tr("C")},  {1, QObject::tr("C♯")},  {2, QObject::tr("D")},
      {3, QObject::tr("E♭")}, {4, QObject::tr("E")},   {5, QObject::tr("F")},
      {6, QObject::tr("F♯")}, {7, QObject::tr("G")},   {8, QObject::tr("A♭")},
      {9, QObject::tr("A")},  {10, QObject::tr("B♭")}, {11, QObject::tr("B")},
  };

  QString result;
  QTextStream stream(&result);
  stream << key << QObject::tr(" Hz ≈ ") << degrees_to_name[to_int(degree)]
         << octave;
  if (cents != 0) {
    stream << QObject::tr(cents >= 0 ? " + " : " − ") << abs(cents)
           << QObject::tr(" cents");
  }
  stream << QObject::tr("; Velocity ")
         << to_int(play_state.current_velocity * velocity_ratio)
         << QObject::tr("; ") << to_int(play_state.current_tempo)
         << QObject::tr(" bpm; Start at ") << to_int(play_state.current_time)
         << QObject::tr(" ms");
  return result;
}

struct Player {
  // data
  QWidget &parent;

  // play state fields
  QList<double> channel_schedules = QList<double>(NUMBER_OF_MIDI_CHANNELS, 0);
  PlayState play_state;

  double final_time = 0;

  fluid_settings_t &settings = []() -> fluid_settings_t & {
    fluid_settings_t &settings = get_reference(new_fluid_settings());
    const auto cores = std::thread::hardware_concurrency();
    set_fluid_int(settings, "synth.midi-channels", NUMBER_OF_MIDI_CHANNELS);
    if (cores > 0) {
      set_fluid_int(settings, "synth.cpu-cores", static_cast<int>(cores));
    }
#ifdef __linux__
    fluid_settings_setstr(&settings, "audio.driver", "pulseaudio");
#endif
    return settings;
  }();
  fluid_event_t &event = get_reference(new_fluid_event());
  fluid_sequencer_t &sequencer = get_reference(new_fluid_sequencer2(0));
  fluid_synth_t &synth = get_reference(new_fluid_synth(&settings));
  const unsigned int soundfont_id = get_soundfont_id(synth);
  const fluid_seq_id_t sequencer_id =
      fluid_sequencer_register_fluidsynth(&sequencer, &synth);

  fluid_audio_driver_t *audio_driver_pointer =
      make_audio_driver(parent, settings, synth);

  // methods
  explicit Player(QWidget &parent_input) : parent(parent_input) {
    fluid_event_set_dest(&event, sequencer_id);
  }

  ~Player() {
    stop_playing(sequencer, event);
    delete_audio_driver(audio_driver_pointer);
    fluid_sequencer_unregister_client(&sequencer, sequencer_id);
    delete_fluid_sequencer(&sequencer);
    delete_fluid_event(&event);
    delete_fluid_synth(&synth);
    delete_fluid_settings(&settings);
  }

  // prevent moving and copying
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};

static void update_final_time(Player &player, const double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
}

template <NoteInterface SubNote>
[[nodiscard]] static auto play_notes(Player &player, const int chord_number,
                                     const QList<SubNote> &sub_notes,
                                     const int first_note_number,
                                     const int number_of_notes) -> bool {
  auto &play_state = player.play_state;
  auto &parent = player.parent;
  auto &sequencer = player.sequencer;
  auto &event = player.event;
  auto &channel_schedules = player.channel_schedules;
  const auto soundfont_id = player.soundfont_id;

  const auto current_time = play_state.current_time;
  const auto current_velocity = play_state.current_velocity;
  const auto current_tempo = play_state.current_tempo;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    const auto channel_number = static_cast<int>(
        std::distance(std::begin(channel_schedules),
                      std::min_element(std::begin(channel_schedules),
                                       std::end(channel_schedules))));
    const auto &sub_note = sub_notes.at(note_number);

    const auto *program_pointer = sub_note.get_program_pointer(
        parent, play_state, chord_number, note_number);
    if (program_pointer == nullptr) {
      return false;
    }
    const auto &program = get_reference(program_pointer);

    fluid_event_program_select(&event, channel_number, soundfont_id,
                               program.bank_number, program.preset_number);
    send_event_at(sequencer, event, current_time);

    const auto maybe_midi_number =
        sub_note.get_closest_midi(parent, sequencer, event, play_state,
                                  channel_number, chord_number, note_number);
    if (!maybe_midi_number.has_value()) {
      return false;
    }
    auto midi_number = maybe_midi_number.value();

    auto velocity = static_cast<short>(std::round(
        current_velocity * rational_to_double(sub_note.velocity_ratio)));
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location<SubNote>(stream, chord_number, note_number);
      QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
      return false;
    }
    fluid_synth_cc(&player.synth, channel_number, BREATH_ID, velocity);
    fluid_event_noteon(&event, channel_number, midi_number, velocity);
    send_event_at(sequencer, event, current_time);

    const auto end_time =
        current_time + get_milliseconds(current_tempo, sub_note.beats);

    fluid_event_noteoff(&event, channel_number, midi_number);
    send_event_at(sequencer, event, end_time);

    channel_schedules[channel_number] = end_time + MAX_RELEASE_TIME;
  }
  return true;
}

template <NoteInterface SubNote>
[[nodiscard]] static auto
play_all_notes(Player &player, const int chord_number,
               const QList<SubNote> &sub_notes) -> bool {
  return play_notes(player, chord_number, sub_notes, 0,
                    static_cast<int>(sub_notes.size()));
}

template <RowInterface SubRow> struct RowsModel : public QAbstractTableModel {
  QItemSelectionModel *selection_model_pointer = nullptr;

  [[nodiscard]] virtual auto is_valid() const -> bool { return true; };
  [[nodiscard]] virtual auto get_rows() const -> QList<SubRow> & = 0;

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    if (!is_valid()) {
      return 0;
    }
    return static_cast<int>(get_rows().size());
  }

  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override {
    return SubRow::get_number_of_columns();
  }

  [[nodiscard]] auto headerData(const int section,
                                const Qt::Orientation orientation,
                                const int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return SubRow::get_column_name(section);
    case Qt::Vertical:
      return section + 1;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    const auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return SubRow::is_column_editable(index.column())
               ? (uneditable | Qt::ItemIsEditable)
               : uneditable;
  }

  [[nodiscard]] virtual auto
  get_status(const int /*row_number*/) const -> QString {
    return "";
  }

  [[nodiscard]] auto data(const QModelIndex &index,
                          const int role) const -> QVariant override {
    Q_ASSERT(index.isValid());
    const auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      return get_status(row_number);
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return get_rows().at(row_number).get_data(index.column());
    }

    return {};
  }

  // don't inline these functions because they use protected methods
  void set_cell(const QModelIndex &set_index, const QVariant &new_value) {
    const auto row_number = set_index.row();
    const auto column_number = set_index.column();

    get_rows()[row_number].set_data(column_number, new_value);
    dataChanged(set_index, set_index);
    get_reference(selection_model_pointer)
        .select(set_index,
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void set_cells(const QItemSelectionRange &range,
                 const QList<SubRow> &new_rows) {
    Q_ASSERT(range.isValid());

    auto &rows = get_rows();
    const auto number_of_new_rows = new_rows.size();

    const auto &top_left_index = range.topLeft();
    const auto &bottom_right_index = range.bottomRight();

    const auto first_row_number = range.top();
    const auto left_column = range.left();
    const auto right_column = range.right();

    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const auto &new_row = new_rows.at(replace_number);
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(new_row, column_number);
      }
    }
    dataChanged(top_left_index, bottom_right_index);
    get_reference(selection_model_pointer)
        .select(QItemSelection(top_left_index, bottom_right_index),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void delete_cells(const QItemSelectionRange &range) {
    Q_ASSERT(range.isValid());

    const auto &top_left_index = range.topLeft();
    const auto &bottom_right_index = range.bottomRight();

    const auto first_row_number = range.top();
    const auto left_column = range.left();
    const auto right_column = range.right();
    const auto number_of_rows = get_number_of_rows(range);

    auto &rows = get_rows();
    for (auto replace_number = 0; replace_number < number_of_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const SubRow empty_row;
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(empty_row, column_number);
      }
    }
    dataChanged(top_left_index, bottom_right_index);
    get_reference(selection_model_pointer)
        .select(QItemSelection(top_left_index, bottom_right_index),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void insert_xml_rows(const int first_row_number, xmlNode &rows_node) {
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number +
                        static_cast<int>(get_number_of_children(rows_node)) -
                        1);
    xml_to_rows(get_rows(), rows_node);
    endInsertRows();
  }

  void insert_rows(const int first_row_number, const QList<SubRow> &new_rows,
                   const int left_column, const int right_column) {
    auto &rows = get_rows();
    const auto number_of_rows = static_cast<int>(new_rows.size());
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
    get_reference(selection_model_pointer)
        .select(QItemSelection(
                    index(first_row_number, left_column),
                    index(first_row_number + number_of_rows - 1, right_column)),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void insert_row(const int row_number, SubRow new_row) {
    beginInsertRows(QModelIndex(), row_number, row_number);
    auto &rows = get_rows();
    rows.insert(rows.begin() + row_number, std::move(new_row));
    endInsertRows();
    get_reference(selection_model_pointer)
        .select(index(row_number, 0), QItemSelectionModel::Select |
                                          QItemSelectionModel::Clear |
                                          QItemSelectionModel::Rows);
  }

  void remove_rows(const int first_row_number, int number_of_rows) {
    auto &rows = get_rows();
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }
};

template <RowInterface SubRow>
static void clear_rows(RowsModel<SubRow> &rows_model) {
  const auto number_of_rows = rows_model.rowCount(QModelIndex());
  if (number_of_rows > 0) {
    rows_model.remove_rows(0, number_of_rows);
  }
}

[[nodiscard]] static auto get_root(_xmlDoc &document) -> xmlNode & {
  return get_reference(xmlDocGetRootElement(&document));
}

[[nodiscard]] static auto make_tree() -> _xmlDoc & {
  return get_reference(xmlNewDoc(to_xml_string(nullptr)));
}

[[nodiscard]] static auto make_root(_xmlDoc &document,
                                    const char *field_name) -> xmlNode & {
  auto &root_node =
      get_reference(xmlNewNode(nullptr, to_xml_string(field_name)));
  xmlDocSetRootElement(&document, &root_node);
  return get_root(document);
}

template <RowInterface SubRow>
static void
copy_from_model(QMimeData &mime_data, const RowsModel<SubRow> &rows_model,
                const int first_row_number, const int number_of_rows,
                const int left_column, const int right_column) {
  const auto &rows = rows_model.get_rows();

  auto &document = make_tree();
  auto &root_node = make_root(document, "clipboard");
  set_xml_int(root_node, "left_column", left_column);
  set_xml_int(root_node, "right_column", right_column);
  partial_set_xml_rows(root_node, "rows", rows, first_row_number,
                       number_of_rows, left_column, right_column);

  xmlChar *char_buffer = nullptr;
  auto buffer_size = 0;
  xmlDocDumpMemory(&document, &char_buffer, &buffer_size);
  xmlFreeDoc(&document);
  mime_data.setData(
      SubRow::get_cells_mime(),
      reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
          char *>(char_buffer));
}

template <RowInterface SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow> &rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(index.data()), new_value(std::move(new_value_input)) {}

  void undo() override { rows_model.set_cell(index, old_value); }

  void redo() override { rows_model.set_cell(index, new_value); }
};

template <RowInterface SubRow> struct DeleteCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const QItemSelectionRange range;
  const QList<SubRow> old_rows;

  explicit DeleteCells(RowsModel<SubRow> &rows_model_input,
                       QItemSelectionRange range_input)
      : rows_model(rows_model_input), range(std::move(range_input)),
        old_rows(copy_items(rows_model.get_rows(), range.top(),
                            get_number_of_rows(range))) {}

  void undo() override { rows_model.set_cells(range, old_rows); }

  void redo() override { rows_model.delete_cells(range); }
};

template <RowInterface SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int number_of_rows;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;

  explicit SetCells(RowsModel<SubRow> &rows_model_input,
                    const int first_row_number_input,
                    const int number_of_rows_input, const int left_column_input,
                    const int right_column_input, QList<SubRow> old_rows_input,
                    QList<SubRow> new_rows_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        number_of_rows(number_of_rows_input), left_column(left_column_input),
        right_column(right_column_input), old_rows(std::move(old_rows_input)),
        new_rows(std::move(new_rows_input)) {}

  void undo() override {
    rows_model.set_cells(
        QItemSelectionRange(
            rows_model.index(first_row_number, left_column),
            rows_model.index(first_row_number + number_of_rows - 1,
                             right_column)),
        old_rows);
  }

  void redo() override {
    rows_model.set_cells(
        QItemSelectionRange(
            rows_model.index(first_row_number, left_column),
            rows_model.index(first_row_number + number_of_rows - 1,
                             right_column)),
        new_rows);
  }
};

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  Q_ASSERT(mime_type.isValidUtf16());
  if (mime_type == Chord::get_cells_mime()) {
    return RowsModel<Chord>::tr("chords cells");
  }
  if (mime_type == PitchedNote::get_cells_mime()) {
    return RowsModel<PitchedNote>::tr("pitched notes cells");
  }
  if (mime_type == UnpitchedNote::get_cells_mime()) {
    return RowsModel<UnpitchedNote>::tr("unpitched notes cells");
  }
  return mime_type;
}

template <RowInterface SubRow> struct Cells {
  const int left_column;
  const int right_column;
  const QList<SubRow> rows;
  Cells(const int left_column_input, const int right_column_input,
        QList<SubRow> rows_input)
      : left_column(left_column_input), right_column(right_column_input),
        rows(std::move(rows_input)) {}
};

template <RowInterface SubRow>
[[nodiscard]] static auto
parse_clipboard(QWidget &parent,
                const int max_rows = std::numeric_limits<int>::max())
    -> std::optional<Cells<SubRow>> {
  const auto &mime_data =
      get_reference(get_reference(QGuiApplication::clipboard()).mimeData());
  const auto *mime_type = SubRow::get_cells_mime();
  if (!mime_data.hasFormat(mime_type)) {
    const auto formats = mime_data.formats();
    if (formats.empty()) {
      QMessageBox::warning(&parent, QObject::tr("Empty paste error"),
                           QObject::tr("Nothing to paste!"));
      return {};
    };
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Cannot paste ") << get_mime_description(formats[0])
           << QObject::tr(" as ") << get_mime_description(mime_type);
    QMessageBox::warning(&parent, QObject::tr("MIME type error"), message);
    return {};
  }

  const auto &copied_text = mime_data.data(mime_type).toStdString();
  auto *document_pointer = xmlReadMemory(
      copied_text.c_str(), copied_text.size(), nullptr, nullptr, 0);
  if (document_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("Paste error"),
                         QObject::tr("Invalid XML"));
    return {};
  }
  auto &document = get_reference(document_pointer);

  // TODO(brandon): separate clipboard file for chords and notes
  // TODO(brandon): make schema
  if (validate_against(document, SubRow::get_clipboard_schema()) != 0) {
    QMessageBox::warning(&parent, QObject::tr("Validation Error"),
                         QObject::tr("Invalid clipboard file"));
    xmlFreeDoc(&document);
    return {};
  }

  auto &root = get_root(document);
  auto &rows_node = get_child(root, "rows");
  QList<SubRow> new_rows;
  auto left_column = get_xml_int(root, "left_column");
  auto right_column = get_xml_int(root, "right_column");
  partial_xml_to_rows(new_rows, rows_node,
    std::min({get_number_of_children(rows_node), max_rows}));
  xmlFreeDoc(&document);
  return Cells(left_column, right_column, std::move(new_rows));
}

[[nodiscard]] static auto get_selection_model(
    const QAbstractItemView &item_view) -> QItemSelectionModel & {
  return get_reference(item_view.selectionModel());
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_set_cells_command(RowsModel<SubRow> &rows_model,
                       const int first_row_number, const int number_of_rows,
                       const int left_column, const int right_column,
                       QList<SubRow> new_rows) -> QUndoCommand * {
  auto &rows = rows_model.get_rows();
  return new SetCells( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number, number_of_rows, left_column, right_column,
      copy_items(rows, first_row_number, number_of_rows), std::move(new_rows));
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_paste_cells_command(QWidget &parent, const int first_row_number,
                         RowsModel<SubRow> &rows_model) -> QUndoCommand * {
  auto &rows = rows_model.get_rows();
  auto maybe_cells = parse_clipboard<SubRow>(
      parent, static_cast<int>(rows.size()) - first_row_number);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto &cells = maybe_cells.value();
  auto &copy_rows = cells.rows;
  const auto number_copied = static_cast<int>(copy_rows.size());
  return make_set_cells_command(rows_model, first_row_number, number_copied,
                                cells.left_column, cells.right_column,
                                std::move(copy_rows));
}

template <RowInterface SubRow> struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow> &rows_model_input, const int row_number_input,
            SubRow new_row_input = SubRow())
      : rows_model(rows_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)) {}

  void undo() override { rows_model.remove_rows(row_number, 1); }

  void redo() override { rows_model.insert_row(row_number, new_row); }
};

template <RowInterface SubRow>
static void
insert_or_remove(RowsModel<SubRow> &rows_model, const int first_row_number,
                 const QList<SubRow> &new_rows, const int left_column,
                 const int right_column, const bool should_insert) {
  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows, left_column,
                           right_column);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}

template <RowInterface SubRow> struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const int left_column;
  const int right_column;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> &rows_model_input,
                   const int first_row_number_input,
                   QList<SubRow> new_rows_input, const int left_column_input,
                   const int right_column_input, const bool backwards_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), left_column(left_column_input),
        right_column(right_column_input), backwards(backwards_input) {}

  void undo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, left_column,
                     right_column, backwards);
  }

  void redo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, left_column,
                     right_column, !backwards);
  }
};

template <RowInterface SubRow>
[[nodiscard]] static auto
make_paste_insert_command(QWidget &parent, RowsModel<SubRow> &rows_model,
                          const int row_number) -> QUndoCommand * {
  const auto maybe_cells = parse_clipboard<SubRow>(parent);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto &cells = maybe_cells.value();
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, std::move(cells.rows), cells.left_column,
      cells.right_column, false);
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_remove_command(RowsModel<SubRow> &rows_model, const int first_row_number,
                    const int number_of_rows) -> QUndoCommand * {
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(rows_model.get_rows(), first_row_number, number_of_rows), 0,
      SubRow::get_number_of_columns(), true);
}

template <RowInterface SubRow> struct UndoRowsModel : public RowsModel<SubRow> {
  QUndoStack &undo_stack;

  explicit UndoRowsModel(QUndoStack &undo_stack_input)
      : undo_stack(undo_stack_input){};

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             const int role) -> bool override {
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack.push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            *this, index, new_value));
    return true;
  }
};

struct ChordsModel : public UndoRowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input)
      : UndoRowsModel(undo_stack), song(song_input) {}

  [[nodiscard]] auto get_rows() const -> QList<Chord> & override {
    return song.chords;
  };

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    return get_status_text(song, row_number);
  }

  [[nodiscard]] auto setData(const QModelIndex &new_index,
                             const QVariant &new_value,
                             const int role) -> bool override {
    if (role != Qt::EditRole) {
      return false;
    }
    return UndoRowsModel::setData(new_index, new_value, role);
  }
};

template <NoteInterface SubNote>
struct NotesModel : public UndoRowsModel<SubNote> {
  QList<SubNote> *rows_pointer = nullptr;
  int parent_chord_number = -1;

  explicit NotesModel(QUndoStack &undo_stack)
      : UndoRowsModel<SubNote>(undo_stack) {}

  [[nodiscard]] auto is_valid() const -> bool override {
    return rows_pointer != nullptr;
  };

  [[nodiscard]] auto get_rows() const -> QList<SubNote> & override {
    return get_reference(rows_pointer);
  };

  void set_rows_pointer(QList<SubNote> *const new_rows_pointer = nullptr,
                        const int new_parent_chord_number = -1) {
    NotesModel::beginResetModel();
    rows_pointer = new_rows_pointer;
    parent_chord_number = new_parent_chord_number;
    NotesModel::endResetModel();
  }
};

template <NoteInterface SubNote>
static auto make_insert_note(NotesModel<SubNote> &notes_model,
                             const QList<Chord> &chords,
                             const int row_number) -> QUndoCommand * {
  SubNote sub_note;
  sub_note.beats = chords[notes_model.parent_chord_number].beats;
  return new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
      notes_model, row_number, std::move(sub_note));
}

struct PitchedNotesModel : public NotesModel<PitchedNote> {
  Song &song;

  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song_input)
      : NotesModel<PitchedNote>(undo_stack), song(song_input) {}

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    const auto &note = get_reference(rows_pointer).at(row_number);
    return get_status_text(song, parent_chord_number,
                           interval_to_double(note.interval),
                           rational_to_double(note.velocity_ratio));
  }
};

struct UnpitchedNotesModel : public NotesModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack &undo_stack)
      : NotesModel<UnpitchedNote>(undo_stack) {}
};

struct MyTable : public QTableView {
  MyTable() {
    auto &horizontal_header = get_reference(horizontalHeader());
    auto &vertical_header = get_reference(verticalHeader());

    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSizeAdjustPolicy(SizeAdjustPolicy::AdjustToContents);

    horizontal_header.setSectionResizeMode(QHeaderView::Fixed);
    vertical_header.setSectionResizeMode(QHeaderView::Fixed);

    horizontal_header.setStretchLastSection(true);

    setMouseTracking(true);
  }

  [[nodiscard]] auto sizeHint() const -> QSize override {
    const QScrollBar &vertical_scroll_bar = get_reference(verticalScrollBar());
    const QScrollBar &horizontal_scroll_bar =
        get_reference(horizontalScrollBar());
    const auto &viewport_size = viewportSizeHint();
    const auto double_frame_width = 2 * frameWidth();
    return {
        double_frame_width + viewport_size.width() +
            (vertical_scroll_bar.isVisible() ? vertical_scroll_bar.width() : 0),
        double_frame_width + viewport_size.height() +
            (horizontal_scroll_bar.isVisible() ? horizontal_scroll_bar.height()
                                               : 0)};
  }

  [[nodiscard]] auto minimumSizeHint() const -> QSize override {
    return {0, 0};
  }
};

template <RowInterface SubRow>
static void set_model(QAbstractItemView &item_view,
                      RowsModel<SubRow> &rows_model) {
  item_view.setModel(&rows_model);
  rows_model.selection_model_pointer = item_view.selectionModel();
}

struct ChordsTable : public MyTable {
  ChordsModel model;
  ChordsTable(QUndoStack &undo_stack, Song &song)
      : model(ChordsModel(undo_stack, song)) {
    const auto &interval_size = get_minimum_size<IntervalEditor>();
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &instrument_size = get_minimum_size<InstrumentEditor>();
    const auto &percussion_instrument_size =
        get_minimum_size<PercussionInstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(chord_instrument_column, instrument_size.width());
    setColumnWidth(chord_percussion_instrument_column,
                   percussion_instrument_size.width());
    setColumnWidth(chord_interval_column, interval_size.width());
    setColumnWidth(chord_beats_column, rational_width);
    setColumnWidth(chord_velocity_ratio_column, rational_width);
    setColumnWidth(chord_tempo_ratio_column, rational_width);
    resizeColumnToContents(chord_pitched_notes_column);
    resizeColumnToContents(chord_unpitched_notes_column);
    setColumnWidth(chord_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(std::max(
            {instrument_size.height(), percussion_instrument_size.height(),
             rational_size.height(), interval_size.height()}));
  }
};

struct UnpitchedNotesTable : public MyTable {
  UnpitchedNotesModel model;
  explicit UnpitchedNotesTable(QUndoStack &undo_stack)
      : model(UnpitchedNotesModel(undo_stack)) {
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &percussion_instrument_size =
        get_minimum_size<PercussionInstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(unpitched_note_percussion_instrument_column,
                   percussion_instrument_size.width());
    setColumnWidth(unpitched_note_beats_column, rational_width);
    setColumnWidth(unpitched_note_velocity_ratio_column, rational_width);
    setColumnWidth(unpitched_note_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(std::max(
            {rational_size.height(), percussion_instrument_size.height()}));
  }
};

struct PitchedNotesTable : public MyTable {
  PitchedNotesModel model;
  PitchedNotesTable(QUndoStack &undo_stack, Song &song)
      : model(PitchedNotesModel(undo_stack, song)) {
    const auto &interval_size = get_minimum_size<IntervalEditor>();
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &instrument_size = get_minimum_size<InstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(pitched_note_instrument_column, instrument_size.width());
    setColumnWidth(pitched_note_interval_column, interval_size.width());
    setColumnWidth(pitched_note_beats_column, rational_width);
    setColumnWidth(pitched_note_velocity_ratio_column, rational_width);
    setColumnWidth(pitched_note_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(
            std::max({rational_size.height(), instrument_size.height(),
                      interval_size.height()}));
  }
};

struct SwitchColumn : public QWidget {
  RowType current_row_type = chord_type;

  QLabel &editing_text = *(new QLabel(SwitchColumn::tr("Chords")));
  ChordsTable &chords_table;
  PitchedNotesTable &pitched_notes_table;
  UnpitchedNotesTable &unpitched_notes_table;

  QBoxLayout &column_layout = *(new QVBoxLayout(this));

  SwitchColumn(QUndoStack &undo_stack, Song &song)
      : chords_table(*new ChordsTable(undo_stack, song)),
        pitched_notes_table(*new PitchedNotesTable(undo_stack, song)),
        unpitched_notes_table(*new UnpitchedNotesTable(undo_stack)) {
    column_layout.addWidget(&editing_text);
    column_layout.addWidget(&chords_table);
    column_layout.addWidget(&pitched_notes_table);
    column_layout.addWidget(&unpitched_notes_table);
  }
};

[[nodiscard]] static auto
get_table(const SwitchColumn &switch_column) -> QTableView & {
  auto &chords_table = switch_column.chords_table;
  switch (switch_column.current_row_type) {
  case chord_type:
    return chords_table;
  case pitched_note_type:
    return switch_column.pitched_notes_table;
  case unpitched_note_type:
    return switch_column.unpitched_notes_table;
  default:
    Q_ASSERT(false);
    return chords_table;
  }
}

[[nodiscard]] static auto
get_parent_chord_number(const SwitchColumn &switch_column) -> int {
  switch (switch_column.current_row_type) {
  case chord_type:
    return -1;
  case pitched_note_type:
    return switch_column.pitched_notes_table.model.parent_chord_number;
  case unpitched_note_type:
    return switch_column.unpitched_notes_table.model.parent_chord_number;
  default:
    Q_ASSERT(false);
    return -1;
  }
}

[[nodiscard]] static auto get_only_range(const SwitchColumn &switch_column) {
  return get_only(get_selection_model(get_table(switch_column)).selection());
}

static void copy_selection(const SwitchColumn &switch_column) {
  const auto &range = get_only_range(switch_column);
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);
  const auto left_column = range.left();
  const auto right_column = range.right();
  auto &mime_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  switch (switch_column.current_row_type) {
  case chord_type:
    copy_from_model(mime_data, switch_column.chords_table.model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  case pitched_note_type:
    copy_from_model(mime_data, switch_column.pitched_notes_table.model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  case unpitched_note_type:
    copy_from_model(mime_data, switch_column.unpitched_notes_table.model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  get_reference(QGuiApplication::clipboard()).setMimeData(&mime_data);
}

struct SetDouble : public QUndoCommand {
  Song &song;
  fluid_synth_t &synth;
  QDoubleSpinBox &spin_box;
  const ChangeId control_id;
  const double old_value;
  double new_value;

  explicit SetDouble(Song &song_input, fluid_synth_t &synth_input,
                     QDoubleSpinBox &spin_box_input,
                     const ChangeId command_id_input,
                     const double old_value_input, const double new_value_input)
      : song(song_input), synth(synth_input), spin_box(spin_box_input),
        control_id(command_id_input), old_value(old_value_input),
        new_value(new_value_input) {}

  [[nodiscard]] auto id() const -> int override { return control_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);
    new_value =
        get_reference(dynamic_cast<const SetDouble *>(next_command_pointer))
            .new_value;
    return true;
  }

  void undo() override {
    set_double(song, synth, control_id, spin_box, old_value);
  }

  void redo() override {
    set_double(song, synth, control_id, spin_box, new_value);
  }
};

static void add_set_double(QUndoStack &undo_stack, Song &song,
                           fluid_synth_t &synth, QDoubleSpinBox &spin_box,
                           const ChangeId control_id, const double old_value,
                           const double new_value) {
  undo_stack.push(new SetDouble( // NOLINT(cppcoreguidelines-owning-memory)
      song, synth, spin_box, control_id, old_value, new_value));
}

struct SpinBoxes : public QWidget {
  QDoubleSpinBox &gain_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_key_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_velocity_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_tempo_editor = *(new QDoubleSpinBox);
  QFormLayout &spin_boxes_form = *(new QFormLayout(this));

  explicit SpinBoxes(Song &song, fluid_synth_t &synth, QUndoStack &undo_stack) {
    auto &gain_editor = this->gain_editor;
    auto &starting_key_editor = this->starting_key_editor;
    auto &starting_velocity_editor = this->starting_velocity_editor;
    auto &starting_tempo_editor = this->starting_tempo_editor;

    add_control(spin_boxes_form, SpinBoxes::tr("&Gain:"), gain_editor, 0,
                MAX_GAIN, SpinBoxes::tr("/10"), GAIN_STEP, 1);
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &key:"),
                starting_key_editor, 1, MAX_STARTING_KEY, SpinBoxes::tr(" hz"));
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &velocity:"),
                starting_velocity_editor, 1, MAX_VELOCITY,
                SpinBoxes::tr("/127"));
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &tempo:"),
                starting_tempo_editor, 1, MAX_STARTING_TEMPO,
                SpinBoxes::tr(" bpm"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QObject::connect(
        &gain_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth, &gain_editor](double new_value) {
          add_set_double(undo_stack, song, synth, gain_editor, gain_id,
                         fluid_synth_get_gain(&synth), new_value);
        });
    QObject::connect(
        &starting_key_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth, &starting_key_editor](double new_value) {
          add_set_double(undo_stack, song, synth, starting_key_editor,
                         starting_key_id, song.starting_key, new_value);
        });
    QObject::connect(
        &starting_velocity_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth,
         &starting_velocity_editor](double new_value) {
          add_set_double(undo_stack, song, synth, starting_velocity_editor,
                         starting_velocity_id, song.starting_velocity,
                         new_value);
        });
    QObject::connect(
        &starting_tempo_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth, &starting_tempo_editor](double new_value) {
          add_set_double(undo_stack, song, synth, starting_tempo_editor,
                         starting_tempo_id, song.starting_tempo, new_value);
        });

    gain_editor.setValue(DEFAULT_GAIN);
    starting_key_editor.setValue(song.starting_key);
    starting_velocity_editor.setValue(song.starting_velocity);
    starting_tempo_editor.setValue(song.starting_tempo);

    clear_and_clean(undo_stack);
  }
};

static auto check_interval(QWidget &parent_widget,
                           const Interval &interval) -> bool {
  const auto numerator = interval.ratio.numerator;
  const auto denominator = interval.ratio.denominator;
  const auto octave = interval.octave;
  if (std::abs(numerator) > MAX_RATIONAL_NUMERATOR) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Numerator ") << numerator
           << QObject::tr(" greater than maximum ") << MAX_RATIONAL_NUMERATOR;
    QMessageBox::warning(&parent_widget, QObject::tr("Numerator error"),
                         message);
    return false;
  }
  if (std::abs(denominator) > MAX_RATIONAL_DENOMINATOR) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Denominator ") << denominator
           << QObject::tr(" greater than maximum ") << MAX_RATIONAL_DENOMINATOR;
    QMessageBox::warning(&parent_widget, QObject::tr("Denominator error"),
                         message);
    return false;
  }
  if (std::abs(octave) > MAX_OCTAVE) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Octave ") << octave
           << QObject::tr(" (absolutely) greater than maximum ") << MAX_OCTAVE;
    QMessageBox::warning(&parent_widget, QObject::tr("Octave error"), message);
    return false;
  }
  return true;
}

static void update_interval(QUndoStack &undo_stack, SwitchColumn &switch_column,
                            const Interval &interval) {
  const auto &range = get_only_range(switch_column);
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);

  const auto current_row_type = switch_column.current_row_type;
  QUndoCommand *undo_command = nullptr;
  if (current_row_type == chord_type) {
    auto &chords_model = switch_column.chords_table.model;
    auto new_chords =
        copy_items(chords_model.get_rows(), first_row_number, number_of_rows);
    for (auto &chord : new_chords) {
      const auto new_interval = chord.interval * interval;
      if (!check_interval(switch_column, new_interval)) {
        return;
      }
      chord.interval = new_interval;
    }
    undo_command = make_set_cells_command(
        chords_model, first_row_number, number_of_rows, chord_interval_column,
        chord_interval_column, std::move(new_chords));
  } else if (current_row_type == pitched_note_type) {
    auto &pitched_notes_model = switch_column.pitched_notes_table.model;
    auto new_pitched_notes = copy_items(pitched_notes_model.get_rows(),
                                        first_row_number, number_of_rows);
    for (auto &pitched_note : new_pitched_notes) {
      const auto new_interval = pitched_note.interval * interval;
      if (!check_interval(switch_column, new_interval)) {
        return;
      }
      pitched_note.interval = new_interval;
    }
    undo_command = make_set_cells_command(
        pitched_notes_model, first_row_number, number_of_rows,
        pitched_note_interval_column, pitched_note_interval_column,
        std::move(new_pitched_notes));
  } else {
    Q_ASSERT(false);
  }
  undo_stack.push(undo_command);
}

static void make_square(QPushButton &button) {
  button.setFixedWidth(button.sizeHint().height());
}

struct IntervalRow : public QWidget {
  QUndoStack &undo_stack;
  SwitchColumn &switch_column;
  QBoxLayout &row_layout = *(new QHBoxLayout(this));
  QPushButton &minus_button = *(new QPushButton("−", this));
  QLabel &text;
  QPushButton &plus_button = *(new QPushButton("+", this));
  const Interval interval;

  IntervalRow(QUndoStack &undo_stack_input, SwitchColumn &switch_column_input,
              const char *const interval_name, Interval interval_input)
      : undo_stack(undo_stack_input), switch_column(switch_column_input),
        text(*(new QLabel(interval_name, this))), interval(interval_input) {
    make_square(minus_button);
    make_square(plus_button);
    row_layout.addWidget(&minus_button);
    row_layout.addWidget(&text);
    row_layout.addWidget(&plus_button);

    auto &switch_column = this->switch_column;
    auto &undo_stack = this->undo_stack;
    const auto &interval = this->interval;

    QObject::connect(&minus_button, &QPushButton::released, this,
                     [&undo_stack, &switch_column, &interval]() {
                       update_interval(undo_stack, switch_column,
                                       Interval() / interval);
                     });

    QObject::connect(&plus_button, &QPushButton::released, this,
                     [&undo_stack, &switch_column, &interval]() {
                       update_interval(undo_stack, switch_column, interval);
                     });
  }
};

static void set_enabled(IntervalRow &interval_row, bool enabled) {
  interval_row.minus_button.setEnabled(enabled);
  interval_row.plus_button.setEnabled(enabled);
}

static void set_rows_enabled(IntervalRow &third_row, IntervalRow &fifth_row,
                             IntervalRow &seventh_row, IntervalRow &octave_row,
                             bool enabled) {
  set_enabled(third_row, enabled);
  set_enabled(fifth_row, enabled);
  set_enabled(seventh_row, enabled);
  set_enabled(octave_row, enabled);
}

struct ControlsColumn : public QWidget {
  SpinBoxes &spin_boxes;
  IntervalRow &third_row;
  IntervalRow &fifth_row;
  IntervalRow &seventh_row;
  IntervalRow &octave_row;
  QBoxLayout &column_layout = *(new QVBoxLayout(this));

  ControlsColumn(Song &song, fluid_synth_t &synth, QUndoStack &undo_stack,
                 SwitchColumn &switch_column)
      : spin_boxes(*new SpinBoxes(song, synth, undo_stack)),
        third_row(*new IntervalRow(undo_stack, switch_column, "Major third",
                                   Interval(Rational(FIVE, 4), 0))),
        fifth_row(*new IntervalRow(undo_stack, switch_column, "Perfect fifth",
                                   Interval(Rational(3, 2), 0))),
        seventh_row(*new IntervalRow(undo_stack, switch_column,
                                     "Harmonic seventh",
                                     Interval(Rational(SEVEN, 4), 0))),
        octave_row(*new IntervalRow(undo_stack, switch_column, "Octave",
                                    Interval(Rational(), 1))) {
    column_layout.addWidget(&spin_boxes);
    column_layout.addWidget(&third_row);
    column_layout.addWidget(&fifth_row);
    column_layout.addWidget(&seventh_row);
    column_layout.addWidget(&octave_row);
    set_rows_enabled(third_row, fifth_row, seventh_row, octave_row, false);
  }
};

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

auto get_gain(const SongWidget &song_widget) -> double {
  return fluid_synth_get_gain(&song_widget.player.synth);
}

[[nodiscard]] static auto get_next_row(const SongWidget &song_widget) {
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

void export_to_file(SongWidget &song_widget, const QString &output_file) {
  Q_ASSERT(output_file.isValidUtf16());
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  auto &settings = player.settings;
  auto &event = player.event;
  auto &sequencer = player.sequencer;

  stop_playing(sequencer, event);
  delete_audio_driver(player.audio_driver_pointer);
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

static void modulate_before_chord(const Song &song, PlayState &play_state,
                                  const int next_chord_number) {
  const auto &chords = song.chords;
  if (next_chord_number > 0) {
    for (auto chord_number = 0; chord_number < next_chord_number;
         chord_number = chord_number + 1) {
      modulate(play_state, chords.at(chord_number));
    }
  }
}

static void add_insert_row(SongWidget &song_widget, const int row_number) {
  auto &switch_column = song_widget.switch_column;
  const auto current_row_type = switch_column.current_row_type;
  QUndoCommand *undo_command = nullptr;
  const auto &chords = song_widget.song.chords;
  switch (current_row_type) {
  case chord_type:
    undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.chords_table.model, row_number);
    break;
  case pitched_note_type:
    undo_command = make_insert_note(switch_column.pitched_notes_table.model,
                                    chords, row_number);
    break;
  case unpitched_note_type:
    undo_command = make_insert_note(switch_column.unpitched_notes_table.model,
                                    chords, row_number);
    break;
  default:
    Q_ASSERT(false);
  }
  song_widget.undo_stack.push(undo_command);
}

static void add_paste_insert(SongWidget &song_widget, const int row_number) {
  auto &switch_column = song_widget.switch_column;
  auto &chords_table = switch_column.chords_table;
  auto &pitched_notes_table = switch_column.pitched_notes_table;
  auto &unpitched_notes_table = switch_column.unpitched_notes_table;

  QUndoCommand *undo_command = nullptr;
  switch (switch_column.current_row_type) {
  case chord_type:
    undo_command =
        make_paste_insert_command(chords_table, chords_table.model, row_number);
    break;
  case pitched_note_type:
    undo_command = make_paste_insert_command(
        pitched_notes_table, pitched_notes_table.model, row_number);
    break;
  case unpitched_note_type:
    undo_command = make_paste_insert_command(
        unpitched_notes_table, unpitched_notes_table.model, row_number);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  if (undo_command == nullptr) {
    return;
  }
  song_widget.undo_stack.push(undo_command);
}

static void add_delete_cells(SongWidget &song_widget) {
  auto &undo_stack = song_widget.undo_stack;
  auto &switch_column = song_widget.switch_column;

  const auto &range = get_only_range(switch_column);

  QUndoCommand *undo_command = nullptr;
  switch (switch_column.current_row_type) {
  case chord_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.chords_table.model, range);
    break;
  case pitched_note_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.pitched_notes_table.model, range);
    break;
  case unpitched_note_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.unpitched_notes_table.model, range);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  undo_stack.push(undo_command);
}

[[nodiscard]] static auto make_file_dialog(
    SongWidget &song_widget, const char *const caption, const QString &filter,
    const QFileDialog::AcceptMode accept_mode, const QString &suffix,
    const QFileDialog::FileMode file_mode) -> QFileDialog & {
  Q_ASSERT(filter.isValidUtf16());
  Q_ASSERT(suffix.isValidUtf16());
  auto &dialog = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&song_widget, SongWidget::tr(caption),
                        song_widget.current_folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

[[nodiscard]] static auto can_discard_changes(SongWidget &song_widget) {
  return song_widget.undo_stack.isClean() ||
         QMessageBox::question(&song_widget, SongWidget::tr("Unsaved changes"),
                               SongWidget::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

[[nodiscard]] static auto get_selected_file(SongWidget &song_widget,
                                            const QFileDialog &dialog) {
  song_widget.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

void save_as_file(SongWidget &song_widget, const QString &filename) {
  Q_ASSERT(filename.isValidUtf16());
  const auto &song = song_widget.song;
  std::ofstream file_io(filename.toStdString().c_str());

  auto &document = make_tree();
  auto &song_node = make_root(document, "song");

  set_xml_double(song_node, "gain", get_gain(song_widget));
  set_xml_double(song_node, "starting_key", song.starting_key);
  set_xml_double(song_node, "starting_tempo", song.starting_tempo);
  set_xml_double(song_node, "starting_velocity", song.starting_velocity);

  set_xml_rows(song_node, "chords", song.chords);

  xmlSaveFile(filename.toStdString().c_str(), &document);
  xmlFreeDoc(&document);

  song_widget.current_file = filename;

  song_widget.undo_stack.setClean();
}

static auto check_xml_document(QWidget &parent, _xmlDoc *document_pointer) {
  // TODO(brandon): handle parse errors!!!
  if (document_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("XML error"),
                         QObject::tr("Invalid XML file"));
    return false;
  }
  return true;
}

static auto maybe_read_xml_file(const QString &filename) {
  // TODO(brandon): handle parse errors!!!
  return xmlReadFile(filename.toStdString().c_str(), nullptr, 0);
}

// TODO(brandon): reuse schemas
static auto validate_against(xmlDoc &document, const char *filename) {
  auto &parser_context = get_reference(
      xmlSchemaNewParserCtxt(get_share_file_name(filename).c_str()));
  auto &schema = get_reference(xmlSchemaParse(&parser_context));
  xmlSchemaFreeParserCtxt(&parser_context);
  auto &validation_context = get_reference(xmlSchemaNewValidCtxt(&schema));

  const auto valid_xml = xmlSchemaValidateDoc(&validation_context, &document);
  xmlSchemaFreeValidCtxt(&validation_context);
  xmlSchemaFree(&schema);
  return valid_xml;
}

void open_file(SongWidget &song_widget, const QString &filename) {

  Q_ASSERT(filename.isValidUtf16());
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &chords_model = song_widget.switch_column.chords_table.model;

  auto *document_pointer = maybe_read_xml_file(filename);
  if (!check_xml_document(song_widget, document_pointer)) {
    return;
  }
  auto &document = get_reference(document_pointer);

  // TODO(brandon): make schema
  if (validate_against(document, "song.xsd") != 0) {
    QMessageBox::warning(&song_widget, QObject::tr("Validation Error"),
                         QObject::tr("Invalid song file"));
    xmlFreeDoc(&document);
    return;
  }

  auto &song_node = get_root(document);

  spin_boxes.gain_editor.setValue(get_xml_double(song_node, "gain"));
  spin_boxes.starting_key_editor.setValue(
      get_xml_double(song_node, "starting_key"));
  spin_boxes.starting_velocity_editor.setValue(
      get_xml_double(song_node, "starting_velocity"));
  spin_boxes.starting_tempo_editor.setValue(
      get_xml_double(song_node, "starting_tempo"));

  clear_rows(chords_model);

  auto *chords_pointer = get_first_child_pointer(song_node, "chords");
  if (chords_pointer != nullptr) {
    chords_model.insert_xml_rows(0, get_reference(chords_pointer));
  }

  xmlFreeDoc(&document);

  song_widget.current_file = filename;

  clear_and_clean(undo_stack);
}

auto get_chords_table(const SongWidget &song_widget) -> QAbstractItemView & {
  return song_widget.switch_column.chords_table;
}

auto get_pitched_notes_table(const SongWidget &song_widget)
    -> QAbstractItemView & {
  return song_widget.switch_column.pitched_notes_table;
}

auto get_unpitched_notes_table(const SongWidget &song_widget)
    -> QAbstractItemView & {
  return song_widget.switch_column.unpitched_notes_table;
}

auto get_starting_key(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_key;
}

auto get_starting_velocity(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_velocity;
}

auto get_starting_tempo(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_tempo;
}

auto get_current_file(const SongWidget &song_widget) -> QString {
  return song_widget.current_file;
}

auto get_current_chord_number(const SongWidget &song_widget) -> int {
  return get_parent_chord_number(song_widget.switch_column);
}

void set_gain(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.gain_editor.setValue(new_value);
}

void set_starting_key(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_key_editor.setValue(
      new_value);
}

void set_starting_velocity(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_velocity_editor.setValue(
      new_value);
}

void set_starting_tempo(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_tempo_editor.setValue(
      new_value);
}

void undo(SongWidget &song_widget) { song_widget.undo_stack.undo(); }

void trigger_third_down(SongWidget &song_widget) {
  song_widget.controls_column.third_row.minus_button.click();
};
void trigger_third_up(SongWidget &song_widget) {
  song_widget.controls_column.third_row.plus_button.click();
};
void trigger_fifth_down(SongWidget &song_widget) {
  song_widget.controls_column.fifth_row.minus_button.click();
};
void trigger_fifth_up(SongWidget &song_widget) {
  song_widget.controls_column.fifth_row.plus_button.click();
};
void trigger_seventh_down(SongWidget &song_widget) {
  song_widget.controls_column.seventh_row.minus_button.click();
};
void trigger_seventh_up(SongWidget &song_widget) {
  song_widget.controls_column.seventh_row.plus_button.click();
};
void trigger_octave_down(SongWidget &song_widget) {
  song_widget.controls_column.octave_row.minus_button.click();
};
void trigger_octave_up(SongWidget &song_widget) {
  song_widget.controls_column.octave_row.plus_button.click();
};

struct MusicXMLNote {
  int start_time = 0;
  int duration = 0;
  int midi_number = 0;
  QString words;
};

struct MusicXMLChord {
  QList<MusicXMLNote> pitched_notes;
  QList<MusicXMLNote> unpitched_notes;
};

struct PartInfo {
  QString part_name;
  QMap<std::string, QString> instrument_map;
  QMap<int, MusicXMLChord> part_chords_dict;
  QMap<int, int> part_divisions_dict;
  QMap<int, int> part_midi_keys_dict;
  QMap<int, int> part_measure_number_dict;
};

[[nodiscard]] static auto get_duration(xmlNode &measure_element) {
  return get_xml_int_default(measure_element, "duration", 0);
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

struct MostRecentIterator {
  QMap<int, int>::iterator state;
  const QMap<int, int>::iterator end;
  int value;

  MostRecentIterator(QMap<int, int> &dict, const int value_input)
      : state(dict.begin()), end(dict.end()), value(value_input) {}
};

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
  if (is_pitched) {
    chord.pitched_notes.push_back(std::move(note));
  } else {
    chord.unpitched_notes.push_back(std::move(note));
  }
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

[[nodiscard]] static auto get_midi_number(xmlNode &pitch, const char *step_name,
                                          const char *octave_name) {
  static const QMap<std::string, int> note_to_midi = {
      {"C", 0},  {"C#", 1}, {"Db", 1},  {"D", 2},   {"D#", 3}, {"Eb", 3},
      {"E", 4},  {"F", 5},  {"F#", 6},  {"Gb", 6},  {"G", 7},  {"G#", 8},
      {"Ab", 8}, {"A", 9},  {"A#", 10}, {"Bb", 10}, {"B", 11}};
  return note_to_midi[get_content(get_child(pitch, step_name))] +
         get_xml_int(pitch, octave_name) * HALFSTEPS_PER_OCTAVE + C_0_MIDI;
}

static auto get_property(xmlNode &node, const char *name) {
  return to_string(xmlGetProp(&node, to_xml_string(name)));
}

static void add_maybe_tied_note(PartInfo &part_info,
                                QMap<int, MusicXMLChord> &chords_dict,
                                QMap<int, MusicXMLNote> &tied_notes,
                                xmlNode &note_element, int note_midi_number,
                                int chord_start_time, int note_duration,
                                bool tie_start, bool tie_end, bool is_pitched) {
  if (tie_end) {
    const auto tied_notes_iterator = tied_notes.find(note_midi_number);
    Q_ASSERT(tied_notes_iterator != tied_notes.end());
    auto &previous_note = tied_notes_iterator.value();
    previous_note.duration = previous_note.duration + note_duration;
    if (!tie_start) {
      add_note_and_maybe_chord(chords_dict, previous_note, is_pitched);
      tied_notes.erase(tied_notes_iterator);
    }
  } else {
    MusicXMLNote new_note;
    new_note.duration = note_duration;
    QTextStream stream(&new_note.words);
    stream << QObject::tr("Part ") << part_info.part_name;
    auto *instrument_pointer =
        get_first_child_pointer(note_element, "instrument");
    if (instrument_pointer != nullptr) {
      stream << QObject::tr(" instrument ")
             << part_info.instrument_map[get_property(
                    get_reference(instrument_pointer), "id")];
    }
    new_note.midi_number = note_midi_number;
    new_note.start_time = chord_start_time;
    if (tie_start) { // also not tie end
      tied_notes[note_midi_number] = std::move(new_note);
    } else { // not tie start or end
      add_note_and_maybe_chord(chords_dict, std::move(new_note), is_pitched);
    }
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

// given a sequence of divisions changes at certain division times
// find a real time and the beats per division
struct TimeIterator {
  const QMap<int, int> dict;
  QMap<int, int>::const_iterator state;
  const QMap<int, int>::const_iterator end;
  const int song_divisions;
  int last_change_time = 0;
  int next_change_divisions_time = 0;
  int time_per_division = 1;

  TimeIterator(QMap<int, int> dict_input, const int song_divisions_input)
      : dict(std::move(dict_input)), state(dict.begin()), end(dict.end()),
        song_divisions(song_divisions_input) {}
};

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
void import_musicxml(SongWidget &song_widget, const QString &filename) {
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &chords_model = song_widget.switch_column.chords_table.model;

  auto *document_pointer = maybe_read_xml_file(filename);
  if (!check_xml_document(song_widget, document_pointer)) {
    return;
  }
  auto &document = get_reference(document_pointer);

  // TODO(brandon): write schema
  // if (validate_against(document, "musicxml.xsd") != 0) {
  //   QMessageBox::warning(&song_widget, QObject::tr("Validation Error"),
  //                        QObject::tr("Invalid musicxml file"));
  //   xmlFreeDoc(&document);
  //   return;
  // }

  // Get root_pointer element
  auto &score_partwise = get_root(document);
  if (!node_is(score_partwise, "score-partwise")) {
    QMessageBox::warning(
        &song_widget, QObject::tr("Partwise error"),
        QObject::tr("Justly only supports partwise musicxml scores"));
    xmlFreeDoc(&document);
    return; // endpoint
  }

  // Get part-list
  auto &part_list = get_child(score_partwise, "part-list");

  QMap<std::string, PartInfo> part_info_dict;
  auto *score_part_pointer = xmlFirstElementChild(&part_list);
  while (score_part_pointer != nullptr) {
    auto &score_part = get_reference(score_part_pointer);
    if (node_is(score_part, "score-part")) {
      PartInfo part_info;
      part_info.part_name =
          get_qstring_content(get_child(score_part, "part-name"));
      auto &instrument_map = part_info.instrument_map;
      auto *score_instrument_pointer = xmlFirstElementChild(score_part_pointer);
      while (score_instrument_pointer != nullptr) {
        auto &score_instrument = get_reference(score_instrument_pointer);
        if (node_is(score_instrument, "score-instrument")) {
          instrument_map[get_property(score_instrument, "id")] =
              get_qstring_content(
                  get_child(score_instrument, "instrument-name"));
        }
        score_instrument_pointer =
            xmlNextElementSibling(score_instrument_pointer);
      }
      part_info_dict[get_property(score_part, "id")] = std::move(part_info);
    }
    score_part_pointer = xmlNextElementSibling(score_part_pointer);
  }

  QMap<int, MusicXMLNote> tied_notes;
  auto song_divisions = 1;

  auto *part_pointer = xmlFirstElementChild(&score_partwise);
  while (part_pointer != nullptr) {
    auto &part = get_reference(part_pointer);
    if (node_is(part, "part")) {
      const auto part_id = get_property(part, "id");
      auto &part_info = part_info_dict[part_id];

      auto &part_chords_dict = part_info.part_chords_dict;
      auto &part_divisions_dict = part_info.part_divisions_dict;
      auto &part_measure_number_dict = part_info.part_measure_number_dict;
      auto &part_midi_keys_dict = part_info.part_midi_keys_dict;

      auto current_time = 0;
      auto chord_start_time = current_time;
      auto measure_number = 1;

      auto *measure_pointer = xmlFirstElementChild(&part);
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
                    FIFTH_HALFSTEPS * get_xml_int(attribute_element, "fifths"));
                part_midi_keys_dict[current_time] = MIDDLE_C_MIDI + degree;
              } else if (attribute_name == "divisions") {
                const auto new_divisions = to_xml_int(attribute_element);
                Q_ASSERT(new_divisions > 0);
                song_divisions = std::lcm(song_divisions, new_divisions);
                part_divisions_dict[current_time] = new_divisions;
              } else if (attribute_name == "transpose") {
                QMessageBox::warning(
                    &song_widget, QObject::tr("Transpose error"),
                    QObject::tr("Transposition not supported"));
                xmlFreeDoc(&document);
                return; // endpoint
              }
              attribute_element_pointer =
                  xmlNextElementSibling(attribute_element_pointer);
            }
          } else if (measure_element_name == "note") {
            const auto note_duration = get_duration(measure_element);
            if (note_duration == 0) {
              QMessageBox::warning(
                  &song_widget, QObject::tr("Note duration error"),
                  QObject::tr("Notes without durations not supported"));
              xmlFreeDoc(&document);
              return; // endpoint
            }

            auto *pitch_pointer =
                get_first_child_pointer(measure_element, "pitch");
            auto *unpitched_pointer =
                get_first_child_pointer(measure_element, "unpitched");

            if (get_first_child_pointer(measure_element, "chord") == nullptr) {
              chord_start_time = current_time;
              current_time += note_duration;
            }

            bool tie_start = false;
            bool tie_end = false;
            auto *tie_pointer = xmlFirstElementChild(measure_element_pointer);
            while (tie_pointer != nullptr) {
              auto tie_node = get_reference(tie_pointer);
              if (node_is(tie_node, "tie")) {
                auto tie_type = get_property(tie_node, "type");
                if (tie_type == "stop") {
                  tie_end = true;
                } else if (tie_type == "start") {
                  tie_start = true;
                }
              }
              tie_pointer = xmlNextElementSibling(tie_pointer);
            }

            if (unpitched_pointer != nullptr) {
              add_maybe_tied_note(
                  part_info, part_chords_dict, tied_notes, measure_element,
                  get_midi_number(get_reference(unpitched_pointer),
                                  "display-step", "display-octave"),
                  chord_start_time, note_duration, tie_start, tie_end, false);
            } else if (pitch_pointer != nullptr) {
              auto &pitch = get_reference(pitch_pointer);
              add_maybe_tied_note(
                  part_info, part_chords_dict, tied_notes, measure_element,
                  get_midi_number(pitch, "step", "octave") +
                      get_xml_int_default(pitch, "alter", 0),
                  chord_start_time, note_duration, tie_start, tie_end, true);
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
    part_pointer = xmlNextElementSibling(part_pointer);
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

  spin_boxes.starting_key_editor.setValue(get_frequency(midi_key));

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

struct FileMenu : public QMenu {
  QAction save_action = QAction(FileMenu::tr("&Save"));
  QAction open_action = QAction(FileMenu::tr("&Open"));
  QAction save_as_action = QAction(FileMenu::tr("&Save As..."));
  QAction import_action = QAction(FileMenu::tr("&Import MusicXML"));
  QAction export_action = QAction(FileMenu::tr("&Export recording"));

  explicit FileMenu(SongWidget &song_widget) : QMenu(FileMenu::tr("&File")) {
    auto &save_action = this->save_action;
    add_menu_action(*this, open_action, QKeySequence::Open);
    add_menu_action(*this, import_action, QKeySequence::StandardKey(), true);
    addSeparator();
    add_menu_action(*this, save_action, QKeySequence::Save, false);
    add_menu_action(*this, save_as_action, QKeySequence::SaveAs);
    add_menu_action(*this, export_action);

    QObject::connect(&song_widget.undo_stack, &QUndoStack::cleanChanged, this,
                     [&save_action, &song_widget]() {
                       save_action.setEnabled(
                           !song_widget.undo_stack.isClean() &&
                           !song_widget.current_file.isEmpty());
                     });

    QObject::connect(&open_action, &QAction::triggered, this, [&song_widget]() {
      if (can_discard_changes(song_widget)) {
        auto &dialog = make_file_dialog(
            song_widget, "Open — Justly", "XML file (*.xml)",
            QFileDialog::AcceptOpen, ".xml", QFileDialog::ExistingFile);
        if (dialog.exec() != 0) {
          open_file(song_widget, get_selected_file(song_widget, dialog));
        }
      }
    });

    QObject::connect(
        &import_action, &QAction::triggered, this, [&song_widget]() {
          if (can_discard_changes(song_widget)) {
            auto &dialog = make_file_dialog(
                song_widget, "Import MusicXML — Justly",
                "MusicXML file (*.musicxml)", QFileDialog::AcceptOpen,
                ".musicxml", QFileDialog::ExistingFile);
            if (dialog.exec() != 0) {
              import_musicxml(song_widget,
                              get_selected_file(song_widget, dialog));
            }
          }
        });

    QObject::connect(&save_action, &QAction::triggered, this, [&song_widget]() {
      save_as_file(song_widget, song_widget.current_file);
    });

    QObject::connect(
        &save_as_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Save As — Justly", "XML file (*.xml)",
              QFileDialog::AcceptSave, ".xml", QFileDialog::AnyFile);

          if (dialog.exec() != 0) {
            save_as_file(song_widget, get_selected_file(song_widget, dialog));
          }
        });

    QObject::connect(
        &export_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Export — Justly", "WAV file (*.wav)",
              QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
          dialog.setLabelText(QFileDialog::Accept, "Export");
          if (dialog.exec() != 0) {
            export_to_file(song_widget, get_selected_file(song_widget, dialog));
          }
        });
  }
};

struct PasteMenu : public QMenu {
  QAction paste_over_action = QAction(PasteMenu::tr("&Over"));
  QAction paste_into_action = QAction(PasteMenu::tr("&Into start"));
  QAction paste_after_action = QAction(PasteMenu::tr("&After"));

  explicit PasteMenu(SongWidget &song_widget) : QMenu(PasteMenu::tr("&Paste")) {
    add_menu_action(*this, paste_over_action, QKeySequence::Paste, false);
    add_menu_action(*this, paste_into_action);
    add_menu_action(*this, paste_after_action, QKeySequence::StandardKey(),
                    false);

    QObject::connect(
        &paste_over_action, &QAction::triggered, this, [&song_widget]() {
          auto &switch_column = song_widget.switch_column;
          auto &chords_table = switch_column.chords_table;
          auto &pitched_notes_table = switch_column.pitched_notes_table;
          auto &unpitched_notes_table = switch_column.unpitched_notes_table;

          const auto first_row_number = get_only_range(switch_column).top();

          QUndoCommand *undo_command = nullptr;
          switch (switch_column.current_row_type) {
          case chord_type:
            undo_command = make_paste_cells_command(
                chords_table, first_row_number, chords_table.model);
            break;
          case pitched_note_type:
            undo_command =
                make_paste_cells_command(pitched_notes_table, first_row_number,
                                         pitched_notes_table.model);
            break;
          case unpitched_note_type:
            undo_command = make_paste_cells_command(
                unpitched_notes_table, first_row_number,
                unpitched_notes_table.model);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          if (undo_command == nullptr) {
            return;
          }
          song_widget.undo_stack.push(undo_command);
        });

    QObject::connect(&paste_into_action, &QAction::triggered, this,
                     [&song_widget]() { add_paste_insert(song_widget, 0); });

    QObject::connect(&paste_after_action, &QAction::triggered, this,
                     [&song_widget]() {
                       add_paste_insert(song_widget, get_next_row(song_widget));
                     });
  }
};

struct InsertMenu : public QMenu {
  QAction insert_after_action = QAction(InsertMenu::tr("&After"));
  QAction insert_into_action = QAction(InsertMenu::tr("&Into start"));

  explicit InsertMenu(SongWidget &song_widget)
      : QMenu(InsertMenu::tr("&Insert")) {
    add_menu_action(*this, insert_after_action,
                    QKeySequence::InsertLineSeparator, false);
    add_menu_action(*this, insert_into_action, QKeySequence::AddTab);

    QObject::connect(&insert_after_action, &QAction::triggered, this,
                     [&song_widget]() {
                       add_insert_row(song_widget, get_next_row(song_widget));
                     });

    QObject::connect(&insert_into_action, &QAction::triggered, this,
                     [&song_widget]() { add_insert_row(song_widget, 0); });
  }
};

struct ViewMenu : public QMenu {
  QAction back_to_chords_action = QAction(ViewMenu::tr("&Back to chords"));
  QAction previous_chord_action = QAction(ViewMenu::tr("&Previous chord"));
  QAction next_chord_action = QAction(ViewMenu::tr("&Next chord"));

  explicit ViewMenu() : QMenu(ViewMenu::tr("&View")) {
    add_menu_action(*this, back_to_chords_action, QKeySequence::Back, false);
    add_menu_action(*this, previous_chord_action, QKeySequence::PreviousChild,
                    false);
    add_menu_action(*this, next_chord_action, QKeySequence::NextChild, false);
  }
};

struct PlayMenu : public QMenu {
  QAction play_action = QAction(PlayMenu::tr("&Play selection"));
  QAction stop_playing_action = QAction(PlayMenu::tr("&Stop playing"));

  explicit PlayMenu(SongWidget &song_widget) : QMenu(PlayMenu::tr("&Play")) {
    add_menu_action(*this, play_action, QKeySequence::Print, false);
    add_menu_action(*this, stop_playing_action, QKeySequence::Cancel);

    const auto &player = song_widget.player;
    QObject::connect(&play_action, &QAction::triggered, this, [&song_widget]() {
      const auto &switch_column = song_widget.switch_column;
      const auto &song = song_widget.song;
      auto &player = song_widget.player;
      auto &play_state = player.play_state;

      const auto current_row_type = switch_column.current_row_type;

      const auto &range = get_only_range(switch_column);
      const auto first_row_number = range.top();
      const auto number_of_rows = get_number_of_rows(range);

      stop_playing(player.sequencer, player.event);
      initialize_play(song_widget);

      if (current_row_type == chord_type) {
        modulate_before_chord(song, play_state, first_row_number);
        play_chords(song_widget, first_row_number, number_of_rows);
      } else {
        const auto chord_number = get_parent_chord_number(switch_column);
        modulate_before_chord(song, play_state, chord_number);
        const auto &chord = song.chords.at(chord_number);
        modulate(play_state, chord);
        if (current_row_type == pitched_note_type) {
          const auto pitched_result =
              play_notes(player, chord_number, chord.pitched_notes,
                         first_row_number, number_of_rows);
          if (!pitched_result) {
            return;
          }
        } else {
          const auto unpitched_result =
              play_notes(player, chord_number, chord.unpitched_notes,
                         first_row_number, number_of_rows);
          if (!unpitched_result) {
            return;
          }
        }
      }
    });

    QObject::connect(
        &stop_playing_action, &QAction::triggered, this,
        [&player]() { stop_playing(player.sequencer, player.event); });
  }
};

struct EditMenu : public QMenu {
  QAction cut_action = QAction(EditMenu::tr("&Cut"));
  QAction copy_action = QAction(EditMenu::tr("&Copy"));
  PasteMenu paste_menu;
  InsertMenu insert_menu;
  QAction delete_cells_action = QAction(EditMenu::tr("&Delete cells"));
  QAction remove_rows_action = QAction(EditMenu::tr("&Remove rows"));

  explicit EditMenu(SongWidget &song_widget)
      : QMenu(EditMenu::tr("&Edit")), paste_menu(PasteMenu(song_widget)),
        insert_menu(InsertMenu(song_widget)) {
    auto &undo_stack = song_widget.undo_stack;
    auto &switch_column = song_widget.switch_column;

    auto &undo_action = get_reference(undo_stack.createUndoAction(this));
    undo_action.setShortcuts(QKeySequence::Undo);

    auto &redo_action = get_reference(undo_stack.createRedoAction(this));
    redo_action.setShortcuts(QKeySequence::Redo);

    addAction(&undo_action);
    addAction(&redo_action);
    addSeparator();

    add_menu_action(*this, cut_action, QKeySequence::Cut);
    add_menu_action(*this, copy_action, QKeySequence::Copy);
    addMenu(&paste_menu);
    addSeparator();

    addMenu(&insert_menu);
    add_menu_action(*this, delete_cells_action, QKeySequence::Delete, false);
    add_menu_action(*this, remove_rows_action, QKeySequence::DeleteStartOfWord,
                    false);
    addSeparator();

    QObject::connect(&cut_action, &QAction::triggered, this, [&song_widget]() {
      copy_selection(song_widget.switch_column);
      add_delete_cells(song_widget);
    });

    QObject::connect(&copy_action, &QAction::triggered, &switch_column,
                     [&switch_column]() { copy_selection(switch_column); });

    QObject::connect(&delete_cells_action, &QAction::triggered, this,
                     [&song_widget]() { add_delete_cells(song_widget); });

    QObject::connect(
        &remove_rows_action, &QAction::triggered, this, [&song_widget]() {
          auto &switch_column = song_widget.switch_column;
          auto &undo_stack = song_widget.undo_stack;

          const auto &range = get_only_range(switch_column);
          const auto first_row_number = range.top();
          const auto number_of_rows = get_number_of_rows(range);

          QUndoCommand *undo_command = nullptr;
          switch (switch_column.current_row_type) {
          case chord_type:
            undo_command =
                make_remove_command(switch_column.chords_table.model,
                                    first_row_number, number_of_rows);
            break;
          case pitched_note_type:
            undo_command =
                make_remove_command(switch_column.pitched_notes_table.model,
                                    first_row_number, number_of_rows);
            break;
          case unpitched_note_type:
            undo_command =
                make_remove_command(switch_column.unpitched_notes_table.model,
                                    first_row_number, number_of_rows);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          undo_stack.push(undo_command);
        });
  }
};

struct SongMenuBar : public QMenuBar {
  FileMenu file_menu;
  EditMenu edit_menu;
  ViewMenu view_menu;
  PlayMenu play_menu;

  explicit SongMenuBar(SongWidget &song_widget)
      : file_menu(FileMenu(song_widget)), edit_menu(EditMenu(song_widget)),
        play_menu(PlayMenu(song_widget)) {
    addMenu(&file_menu);
    addMenu(&edit_menu);
    addMenu(&view_menu);
    addMenu(&play_menu);
  }
};

static void update_actions(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                           const QItemSelectionModel &selector) {
  auto &edit_menu = song_menu_bar.edit_menu;
  auto &controls_column = song_widget.controls_column;

  const auto anything_selected = !selector.selection().empty();
  const auto any_pitched_selected =
      anything_selected &&
      song_widget.switch_column.current_row_type != unpitched_note_type;

  set_rows_enabled(controls_column.third_row, controls_column.fifth_row,
                   controls_column.seventh_row, controls_column.octave_row,
                   any_pitched_selected);

  edit_menu.cut_action.setEnabled(anything_selected);
  edit_menu.copy_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_over_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_after_action.setEnabled(anything_selected);
  edit_menu.insert_menu.insert_after_action.setEnabled(anything_selected);
  edit_menu.delete_cells_action.setEnabled(anything_selected);
  edit_menu.remove_rows_action.setEnabled(anything_selected);
  song_menu_bar.play_menu.play_action.setEnabled(anything_selected);
}

static void set_model(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                      const RowType row_type) {
  auto &switch_column = song_widget.switch_column;
  switch_column.current_row_type = row_type;
  switch_column.chords_table.setVisible(row_type == chord_type);
  switch_column.pitched_notes_table.setVisible(row_type == pitched_note_type);
  switch_column.unpitched_notes_table.setVisible(row_type ==
                                                 unpitched_note_type);
  update_actions(song_menu_bar, song_widget,
                 get_selection_model(get_table(switch_column)));
}

void trigger_back_to_chords(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.back_to_chords_action.trigger();
}

void trigger_insert_after(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
}

void trigger_insert_into(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.insert_menu.insert_into_action.trigger();
}

void trigger_delete_cells(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.delete_cells_action.trigger();
}

void trigger_remove_rows(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.remove_rows_action.trigger();
}

void trigger_cut(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.cut_action.trigger();
}

void trigger_copy(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.copy_action.trigger();
}

void trigger_paste_over(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();
}

void trigger_paste_into(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_into_action.trigger();
}

void trigger_paste_after(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_after_action.trigger();
}

void trigger_previous_chord(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.previous_chord_action.trigger();
}

void trigger_next_chord(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.next_chord_action.trigger();
}

void trigger_save(SongMenuBar &song_menu_bar) {
  song_menu_bar.file_menu.save_action.trigger();
}

void trigger_play(SongMenuBar &song_menu_bar) {
  song_menu_bar.play_menu.play_action.trigger();
}

void trigger_stop_playing(SongMenuBar &song_menu_bar) {
  song_menu_bar.play_menu.stop_playing_action.trigger();
}

static void replace_table(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                          const RowType new_row_type,
                          const int new_chord_number) {
  auto &switch_column = song_widget.switch_column;
  auto &view_menu = song_menu_bar.view_menu;

  auto &previous_chord_action = view_menu.previous_chord_action;
  auto &next_chord_action = view_menu.next_chord_action;

  auto &chords = song_widget.song.chords;
  auto to_chords = new_row_type == chord_type;

  const auto old_row_type = switch_column.current_row_type;
  const auto row_type_changed = old_row_type != new_row_type;

  QString label_text;
  QTextStream stream(&label_text);
  switch (new_row_type) {
  case chord_type:
    stream << SongMenuBar::tr("Chords");
    break;
  case pitched_note_type:
    stream << SongMenuBar::tr("Pitched notes for chord ")
           << new_chord_number + 1;
    break;
  case unpitched_note_type:
    stream << SongMenuBar::tr("Unpitched notes for chord ")
           << new_chord_number + 1;
  }
  switch_column.editing_text.setText(label_text);

  if (row_type_changed) {
    song_menu_bar.view_menu.back_to_chords_action.setEnabled(!to_chords);
    song_menu_bar.file_menu.open_action.setEnabled(to_chords);
  }

  if (to_chords) {
    previous_chord_action.setEnabled(false);
    next_chord_action.setEnabled(false);

    auto &chords_table = switch_column.chords_table;
    const auto chord_index =
        chords_table.model.index(get_parent_chord_number(switch_column), 0);
    get_selection_model(chords_table)
        .select(chord_index, QItemSelectionModel::Select |
                                 QItemSelectionModel::Clear |
                                 QItemSelectionModel::Rows);
    chords_table.scrollTo(chord_index);

    set_model(song_menu_bar, song_widget, new_row_type);
    if (old_row_type == pitched_note_type) {
      switch_column.pitched_notes_table.model.set_rows_pointer();
    } else {
      switch_column.unpitched_notes_table.model.set_rows_pointer();
    }
  } else {
    auto &chord = chords[new_chord_number];
    previous_chord_action.setEnabled(new_chord_number > 0);
    next_chord_action.setEnabled(new_chord_number < chords.size() - 1);
    if (new_row_type == pitched_note_type) {
      switch_column.pitched_notes_table.model.set_rows_pointer(
          &chord.pitched_notes, new_chord_number);
    } else {
      switch_column.unpitched_notes_table.model.set_rows_pointer(
          &chord.unpitched_notes, new_chord_number);
    }
    set_model(song_menu_bar, song_widget, new_row_type);
  }
}

struct ReplaceTable : public QUndoCommand {
  SongMenuBar &song_menu_bar;
  SongWidget &song_widget;
  const RowType old_row_type;
  const int old_chord_number;
  RowType new_row_type;
  int new_chord_number;

  explicit ReplaceTable(SongMenuBar &song_menu_bar_input,
                        SongWidget &song_widget_input,
                        const RowType new_row_type_input,
                        const int new_chord_number_input)
      : song_menu_bar(song_menu_bar_input), song_widget(song_widget_input),
        old_row_type(song_widget.switch_column.current_row_type),
        old_chord_number(get_parent_chord_number(song_widget.switch_column)),
        new_row_type(new_row_type_input),
        new_chord_number(new_chord_number_input){};

  [[nodiscard]] auto id() const -> int override { return replace_table_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);
    const auto &next_command =
        get_reference(dynamic_cast<const ReplaceTable *>(next_command_pointer));
    const auto next_row_type = next_command.new_row_type;
    const auto next_chord_number = next_command.new_chord_number;
    if (old_row_type == next_row_type &&
        old_chord_number == next_chord_number) {
      setObsolete(true);
    }
    new_row_type = next_row_type;
    new_chord_number = next_chord_number;
    return true;
  }

  void undo() override {
    replace_table(song_menu_bar, song_widget, old_row_type, old_chord_number);
  }

  void redo() override {
    replace_table(song_menu_bar, song_widget, new_row_type, new_chord_number);
  }
};

static void add_replace_table(SongMenuBar &song_menu_bar,
                              SongWidget &song_widget,
                              const RowType new_row_type,
                              const int new_chord_number) {
  song_widget.undo_stack.push(
      new ReplaceTable( // NOLINT(cppcoreguidelines-owning-memory)
          song_menu_bar, song_widget, new_row_type, new_chord_number));
}

static void connect_selection_model(SongMenuBar &song_menu_bar,
                                    SongWidget &song_widget,
                                    const QAbstractItemView &table) {
  const auto &selection_model = get_selection_model(table);
  update_actions(song_menu_bar, song_widget, selection_model);
  QObject::connect(
      &selection_model, &QItemSelectionModel::selectionChanged,
      &selection_model, [&song_menu_bar, &song_widget, &selection_model]() {
        update_actions(song_menu_bar, song_widget, selection_model);
      });
}

SongEditor::SongEditor()
    : song_widget(*(new SongWidget)),
      song_menu_bar(*(new SongMenuBar(song_widget))) {
  auto &song_menu_bar = this->song_menu_bar;
  auto &song_widget = this->song_widget;

  auto &switch_column = song_widget.switch_column;
  auto &undo_stack = song_widget.undo_stack;

  get_reference(statusBar()).showMessage("");

  setWindowTitle("Justly");
  setCentralWidget(&song_widget);
  setMenuBar(&song_menu_bar);
  resize(QSize(get_reference(QGuiApplication::primaryScreen())
                   .availableGeometry()
                   .size()
                   .width(),
               minimumSizeHint().height()));

  connect_selection_model(song_menu_bar, song_widget,
                          switch_column.chords_table);
  connect_selection_model(song_menu_bar, song_widget,
                          switch_column.pitched_notes_table);
  connect_selection_model(song_menu_bar, song_widget,
                          switch_column.unpitched_notes_table);

  QObject::connect(&song_menu_bar.view_menu.back_to_chords_action,
                   &QAction::triggered, this, [&song_menu_bar, &song_widget]() {
                     add_replace_table(song_menu_bar, song_widget, chord_type,
                                       -1);
                   });

  QObject::connect(
      &switch_column.chords_table, &QAbstractItemView::doubleClicked, this,
      [&song_menu_bar, &song_widget](const QModelIndex &index) {
        const auto column = index.column();
        const auto is_pitched = column == chord_pitched_notes_column;
        if (is_pitched || (column == chord_unpitched_notes_column)) {
          add_replace_table(
              song_menu_bar, song_widget,
              (is_pitched ? pitched_note_type : unpitched_note_type),
              index.row());
        }
      });

  QObject::connect(&song_menu_bar.view_menu.previous_chord_action,
                   &QAction::triggered, this, [&song_menu_bar, &song_widget]() {
                     const auto &switch_column = song_widget.switch_column;
                     add_replace_table(song_menu_bar, song_widget,
                                       switch_column.current_row_type,
                                       get_parent_chord_number(switch_column) -
                                           1);
                   });
  QObject::connect(&song_menu_bar.view_menu.next_chord_action,
                   &QAction::triggered, this, [&song_menu_bar, &song_widget]() {
                     const auto &switch_column = song_widget.switch_column;
                     add_replace_table(song_menu_bar, song_widget,
                                       switch_column.current_row_type,
                                       get_parent_chord_number(switch_column) +
                                           1);
                   });

  add_replace_table(song_menu_bar, song_widget, chord_type, -1);
  clear_and_clean(undo_stack);
}

void SongEditor::closeEvent(QCloseEvent *const close_event_pointer) {
  if (!can_discard_changes(song_widget)) {
    get_reference(close_event_pointer).ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
};

// TODO(brandon): reduce iteration over xml nodes
// TODO(brandon): add docs for buttons
// TODO(brandon): instrument mapping for musicxml
// TODO(brandon): musicxml repeats
// TODO(brandon): error on button OOB

#include "justly.moc"