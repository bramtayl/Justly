#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QHeaderView>
#include <QIcon>
#include <QItemEditorFactory>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaType>
#include <QMimeData>
#include <QObject>
#include <QRect>
#include <QScreen>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QStringListModel>
#include <QStyleOption>
#include <QTableView>
#include <QTextStream>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fluidsynth.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "justly/justly.hpp"

enum ControlId {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id
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

enum ModelType { chords_type, pitched_notes_type, unpitched_notes_type };

static const auto BEND_PER_HALFSTEP = 4096;
static const auto C_0_MIDI = 12;
static const auto CENTS_PER_HALFSTEP = 100;
static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;
static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;
static const auto GAIN_STEP = 0.1;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_GAIN = 10;
static const auto MAX_OCTAVE = 9;
static const auto MAX_RATIONAL_DENOMINATOR = 199;
static const auto MAX_RATIONAL_NUMERATOR = 199;
static const auto MAX_STARTING_KEY = 440;
static const auto MAX_STARTING_TEMPO = 200;
static const auto MAX_VELOCITY = 127;
static const auto MIN_STARTING_KEY = 60;
static const auto MIN_STARTING_TEMPO = 25;
static const auto NUMBER_OF_MIDI_CHANNELS = 16;
static const auto OCTAVE_RATIO = 2.0;
static const auto MILLISECONDS_PER_MINUTE = 60000;
static const auto START_END_MILLISECONDS = 500;
static const auto VERBOSE_FLUIDSYNTH = false;
static const auto ZERO_BEND_HALFSTEPS = 2;

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Thing>
[[nodiscard]] static auto
get_const_reference(const Thing *thing_pointer) -> const Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Iterable>
[[nodiscard]] static auto get_only(const Iterable &iterable) {
  Q_ASSERT(iterable.size() == 1);
  return iterable.at(0);
}

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     int first_row_number, int number_of_rows) {
  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

[[nodiscard]] static auto to_int(double value) {
  return static_cast<int>(std::round(value));
}

template <typename SubType>
[[nodiscard]] static auto variant_to(const QVariant &variant) {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}

[[nodiscard]] static auto get_json_value(const nlohmann::json &json_data,
                                         const char *field) {
  Q_ASSERT(json_data.is_object());
  Q_ASSERT(json_data.contains(field));
  return json_data[field];
}

[[nodiscard]] static auto get_json_int(const nlohmann::json &json_data,
                                       const char *field) {
  const auto &json_value = get_json_value(json_data, field);
  Q_ASSERT(json_value.is_number_integer());
  return json_value.get<int>();
}

static auto get_json_double(const nlohmann::json &json_song,
                            const char *field_name) -> double {
  const auto &json_value = get_json_value(json_song, field_name);
  Q_ASSERT(json_value.is_number());
  return json_value.get<double>();
}

static void add_int_to_json(nlohmann::json &json_object, const char *field_name,
                            int value, int default_value) {
  if (value != default_value) {
    json_object[field_name] = value;
  }
}

static void add_words_to_json(nlohmann::json &json_row, const QString &words) {
  if (!words.isEmpty()) {
    json_row["words"] = words.toStdString().c_str();
  }
}

[[nodiscard]] static auto get_number_schema(const char *type, int minimum,
                                            int maximum) {
  return nlohmann::json(
      {{"type", type}, {"minimum", minimum}, {"maximum", maximum}});
}

[[nodiscard]] static auto get_rational_fields_schema() {
  return nlohmann::json(
      {{"numerator", get_number_schema("integer", 1, MAX_RATIONAL_NUMERATOR)},
       {"denominator",
        get_number_schema("integer", 1, MAX_RATIONAL_DENOMINATOR)}});
}

[[nodiscard]] static auto
get_object_schema(const nlohmann::json &properties_json) {
  return nlohmann::json({{"type", "object"}, {"properties", properties_json}});
}

[[nodiscard]] static auto
make_validator(const nlohmann::json &required_json,
               const nlohmann::json &properties_json) {
  return nlohmann::json_schema::json_validator(
      nlohmann::json({{"type", "object"},
                      {"&schema", "http://json-schema.org/draft-07/schema#"},
                      {"required", required_json},
                      {"properties", properties_json}}));
}

[[nodiscard]] static auto get_midi(double key) {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

static void check_fluid_ok(int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

static void set_fluid_int(fluid_settings_t *settings_pointer, const char *field,
                          int value) {
  check_fluid_ok(fluid_settings_setint(settings_pointer, field, value));
}

[[nodiscard]] auto get_soundfont_id(fluid_synth_t *synth_pointer) {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath("../share/MuseScore_General.sf2")
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

static void send_event_at(fluid_sequencer_t *sequencer_pointer,
                          fluid_event_t *event_pointer, double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1));
}

static void show_warning(QWidget &parent, const char *title,
                         const QString &message) {
  QMessageBox::warning(&parent, QObject::tr(title), message);
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

[[nodiscard]] static auto get_clipboard() -> QClipboard & {
  return get_reference(QGuiApplication::clipboard());
}

[[nodiscard]] static auto make_action(const char *name) {
  return QAction(QObject::tr(name));
}

[[nodiscard]] static auto
make_file_dialog(QWidget &parent, const char *caption, const QString &folder,
                 const QString &filter, QFileDialog::AcceptMode accept_mode,
                 const QString &suffix,
                 QFileDialog::FileMode file_mode) -> QFileDialog & {
  auto &dialog = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&parent, QObject::tr(caption), folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

static void prevent_compression(QWidget &widget) {
  widget.setMinimumSize(widget.minimumSizeHint());
}

static void add_menu_action(
    QMenu &menu, QAction &action,
    QKeySequence::StandardKey key_sequence = QKeySequence::StandardKey(),
    bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}

static void add_control(QFormLayout &controls_form, const char *label,
                        QDoubleSpinBox &spin_box, int minimum, int maximum,
                        const char *suffix, double single_step = 1,
                        int decimals = 0) {
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  spin_box.setSuffix(QObject::tr(suffix));
  controls_form.addRow(QWidget::tr(label), &spin_box);
}

[[nodiscard]] static auto can_discard_changes(QWidget &parent,
                                              const QUndoStack &undo_stack) {
  return undo_stack.isClean() ||
         QMessageBox::question(&parent, QObject::tr("Unsaved changes"),
                               QObject::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

// a subnamed should have the following methods:
// static auto SubNamed::get_all_nameds() -> const QList<SubNamed>&;
// static auto get_field_name() -> const char*;
struct Named {
  QString name;

  explicit Named(const char *name_input) : name(name_input){};
};

[[nodiscard]] static auto get_name_or_empty(const Named *named_pointer) {
  if (named_pointer == nullptr) {
    return QString("");
  }
  return named_pointer->name;
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto get_by_name(const QString &name) -> const SubNamed & {
  const auto &all_nameds = SubNamed::get_all_nameds();
  const auto named_pointer =
      std::find_if(all_nameds.cbegin(), all_nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <std::derived_from<Named> SubNamed>
static void add_named_to_json(nlohmann::json &json_row,
                              const SubNamed *named_pointer) {
  if (named_pointer != nullptr) {
    json_row[SubNamed::get_field_name()] = named_pointer->name.toStdString();
  }
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto json_field_to_named_pointer(
    const nlohmann::json &json_row) -> const SubNamed * {
  const char *field_name = SubNamed::get_field_name();
  if (json_row.contains(field_name)) {
    const auto &json_named = json_row[field_name];
    Q_ASSERT(json_named.is_string());
    return &get_by_name<SubNamed>(
        QString::fromStdString(json_named.get<std::string>()));
  };
  return nullptr;
}

template <std::derived_from<Named> SubNamed>
static void add_named_schema(nlohmann::json &json_row) {
  std::vector<std::string> names;
  const auto &all_nameds = SubNamed::get_all_nameds();
  std::transform(all_nameds.cbegin(), all_nameds.cend(),
                 std::back_inserter(names),
                 [](const SubNamed &item) { return item.name.toStdString(); });
  json_row[SubNamed::get_field_name()] =
      nlohmann::json({{"type", "string"}, {"enum", std::move(names)}});
}

struct PercussionInstrument : public Named {
  short midi_number;

  PercussionInstrument(const char *name, short midi_number_input)
      : Named(name), midi_number(midi_number_input) {}

  [[nodiscard]] static auto
  get_all_nameds() -> const QList<PercussionInstrument> & {
    static const QList<PercussionInstrument> all_percussions({
        PercussionInstrument("Acoustic or Low Bass Drum", 35),
        PercussionInstrument("Acoustic Snare", 38),
        PercussionInstrument("Belltree", 84),
        PercussionInstrument("Castanets", 85),
        PercussionInstrument("Chinese Cymbal", 52),
        PercussionInstrument("Closed Hi-hat", 42),
        PercussionInstrument("Cowbell", 56),
        PercussionInstrument("Crash Cymbal 1", 49),
        PercussionInstrument("Crash Cymbal 2", 57),
        PercussionInstrument("Drum sticks", 31),
        PercussionInstrument("Electric or High Bass Drum", 36),
        PercussionInstrument("Electric Snare or Rimshot", 40),
        PercussionInstrument("Hand Clap", 39),
        PercussionInstrument("High Floor Tom", 43),
        PercussionInstrument("High Q or Filter Snap", 27),
        PercussionInstrument("High Tom", 50),
        PercussionInstrument("High-Mid Tom", 48),
        PercussionInstrument("Jingle Bell", 83),
        PercussionInstrument("Low Floor Tom", 41),
        PercussionInstrument("Low Tom", 45),
        PercussionInstrument("Low-Mid Tom", 47),
        PercussionInstrument("Metronome Bell", 34),
        PercussionInstrument("Metronome Click", 33),
        PercussionInstrument("Mute Surdo", 86),
        PercussionInstrument("Open Hi-hat", 46),
        PercussionInstrument("Open Surdo", 87),
        PercussionInstrument("Pedal Hi-hat", 44),
        PercussionInstrument("Ride Bell", 53),
        PercussionInstrument("Ride Cymbal 1", 51),
        PercussionInstrument("Scratch Pull", 30),
        PercussionInstrument("Scratch Push", 29),
        PercussionInstrument("Shaker", 82),
        PercussionInstrument("Side Stick", 37),
        PercussionInstrument("Slap Noise", 28),
        PercussionInstrument("Splash Cymbal", 55),
        PercussionInstrument("Square Click", 32),
        PercussionInstrument("Tambourine", 54),
    });
    return all_percussions;
  }

  [[nodiscard]] static auto get_field_name() { return "percussion_instrument"; }
};

Q_DECLARE_METATYPE(const PercussionInstrument *);

struct Program : public Named {
  short bank_number;
  short preset_number;

  Program(const char *name, short bank_number_input, short preset_number_input)
      : Named(name), bank_number(bank_number_input),
        preset_number(preset_number_input){};
};

template <std::derived_from<Program> SubProgram>
[[nodiscard]] static auto get_programs(bool is_percussion) {
  QList<SubProgram> programs;
  auto *settings_pointer = new_fluid_settings();
  auto *synth_pointer = new_fluid_synth(settings_pointer);

  fluid_sfont_t *soundfont_pointer = fluid_synth_get_sfont_by_id(
      synth_pointer, get_soundfont_id(synth_pointer));
  Q_ASSERT(soundfont_pointer != nullptr);

  fluid_sfont_iteration_start(soundfont_pointer);
  auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  static const std::set<QString> skip_names(
      {"Marching Snare", "OldMarchingBass", "Marching Cymbals", "Marching Bass",
       "OldMarchingTenor", "Marching Tenor",
       // dummy programs
       "Basses Fast", "Basses Pizzicato", "Basses Slow", "Basses Tremolo",
       "Celli Fast", "Celli Pizzicato", "Celli Slow", "Celli Tremolo",
       "Violas Fast", "Violas Pizzicato", "Violas Slow", "Violas Tremolo",
       "Violins Fast", "Violins Pizzicato", "Violins Slow", "Violins Tremolo",
       "Violins2 Fast", "Violins2 Pizzicato", "Violins2 Slow",
       "Violins2 Tremolo",
       // expressive programs
       "5th Saw Wave Expr.", "Accordion Expr.", "Alto Sax Expr.",
       "Atmosphere Expr.", "Bandoneon Expr.", "Baritone Sax Expr.",
       "Bass & Lead Expr.", "Basses Fast Expr.", "Basses Slow Expr.",
       "Basses Trem Expr.", "Bassoon Expr.", "Bottle Chiff Expr.",
       "Bowed Glass Expr.", "Brass 2 Expr.", "Brass Section Expr.",
       "Calliope Lead Expr.", "Celli Fast Expr.", "Celli Slow Expr.",
       "Celli Trem Expr.", "Cello Expr.", "Charang Expr.", "Chiffer Lead Expr.",
       "Choir Aahs Expr.", "Church Organ 2 Expr.", "Church Organ Expr.",
       "Clarinet Expr.", "Contrabass Expr.", "Detuned Org. 1 Expr.",
       "Detuned Org. 2 Expr.", "Detuned Saw Expr.", "Drawbar Organ Expr.",
       "Echo Drops Expr.", "English Horn Expr.", "Fiddle Expr.", "Flute Expr.",
       "French Horns Expr.", "Goblin Expr.", "Halo Pad Expr.",
       "Harmonica Expr.", "Hmn. Mute Tpt. Expr.", "It. Accordion Expr.",
       "Metal Pad Expr.", "Oboe Expr.", "Ocarina Expr.", "Pan Flute Expr.",
       "Perc. Organ Expr.", "Piccolo Expr.", "Polysynth Expr.",
       "Recorder Expr.", "Reed Organ Expr.", "Rock Organ Expr.",
       "Saw Lead Expr.", "Shakuhachi Expr.", "Shenai Expr.", "Sine Wave Expr.",
       "Slow Violin Expr.", "Solo Vox Expr.", "Soprano Sax Expr.",
       "Soundtrack Expr.", "Space Voice Expr.", "Square Lead Expr.",
       "Star Theme Expr.", "Strings Fast Expr.", "Strings Slow Expr.",
       "Strings Trem Expr.", "Sweep Pad Expr.", "Syn. Strings 1 Expr.",
       "Syn. Strings 2 Expr.", "Synth Brass 1 Expr.", "Synth Brass 2 Expr.",
       "Synth Brass 3 Expr.", "Synth Brass 4 Expr.", "Synth Strings3 Expr.",
       "Synth Voice Expr.", "Tenor Sax Expr.", "Trombone Expr.",
       "Trumpet Expr.", "Tuba Expr.", "Viola Expr.", "Violas Fast Expr.",
       "Violas Slow Expr.", "Violas Trem Expr.", "Violin Expr.",
       "Violins Fast Expr.", "Violins Slow Expr.", "Violins Trem Expr.",
       "Violins2 Fast Expr.", "Violins2 Slow Expr.", "Violins2 Trem Expr.",
       "Voice Oohs Expr.", "Warm Pad Expr.", "Whistle Expr.",
       // not working?
       "Temple Blocks"});

  static const std::set<QString> percussion_set_names(
      {"Brush 1",    "Brush 2",    "Brush",      "Electronic", "Jazz 1",
       "Jazz 2",     "Jazz 3",     "Jazz 4",     "Jazz",       "Orchestra Kit",
       "Power 1",    "Power 2",    "Power 3",    "Power",      "Room 1",
       "Room 2",     "Room 3",     "Room 4",     "Room 5",     "Room 6",
       "Room 7",     "Room",       "Standard 1", "Standard 2", "Standard 3",
       "Standard 4", "Standard 5", "Standard 6", "Standard 7", "Standard",
       "TR-808"});

  while (preset_pointer != nullptr) {
    const auto *name = fluid_preset_get_name(preset_pointer);
    if (!skip_names.contains(name) &&
        is_percussion == percussion_set_names.contains(name)) {
      programs.push_back(SubProgram(
          name, static_cast<short>(fluid_preset_get_banknum(preset_pointer)),
          static_cast<short>(fluid_preset_get_num(preset_pointer))));
    }
    preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  }

  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);

  std::sort(programs.begin(), programs.end(),
            [](const SubProgram &instrument_1, const SubProgram &instrument_2) {
              return instrument_1.name <= instrument_2.name;
            });

  return programs;
}

struct PercussionSet : public Program {
  PercussionSet(const char *name, short bank_number, short preset_number)
      : Program(name, bank_number, preset_number){};

  [[nodiscard]] static auto get_all_nameds() -> const QList<PercussionSet> & {
    static const auto all_percussion_sets = get_programs<PercussionSet>(true);
    return all_percussion_sets;
  }

  [[nodiscard]] static auto get_field_name() { return "percussion_set"; };
};

Q_DECLARE_METATYPE(const PercussionSet *);

struct Instrument : public Program {
  Instrument(const char *name, short bank_number, short preset_number)
      : Program(name, bank_number, preset_number){};

  [[nodiscard]] static auto get_all_nameds() -> const QList<Instrument> & {
    static const auto all_instruments = get_programs<Instrument>(false);
    return all_instruments;
  }

  [[nodiscard]] static auto get_field_name() { return "instrument"; };
};

Q_DECLARE_METATYPE(const Instrument *);

// a sub rational should have the following method:
// SubRational(const nlohmann::json &json_rational);
struct AbstractRational {
  int numerator = 1;
  int denominator = 1;

  AbstractRational() = default;
  AbstractRational(int numerator_input, int denominator_input)
      : numerator(numerator_input), denominator(denominator_input) {}

  explicit AbstractRational(const nlohmann::json &json_rational)
      : numerator(json_rational.value("numerator", 1)),
        denominator(json_rational.value("denominator", 1)) {}

  virtual ~AbstractRational() = default;

  [[nodiscard]] auto operator==(const AbstractRational &other_rational) const {
    return numerator == other_rational.numerator &&
           denominator == other_rational.denominator;
  }

  [[nodiscard]] virtual auto is_default() const -> bool {
    return numerator == 1 && denominator == 1;
  }

  [[nodiscard]] virtual auto to_double() const -> double {
    Q_ASSERT(denominator != 0);
    return 1.0 * numerator / denominator;
  }

  virtual void to_json(nlohmann::json &json_rational) const {
    add_int_to_json(json_rational, "numerator", numerator, 1);
    add_int_to_json(json_rational, "denominator", denominator, 1);
  }
};

template <std::derived_from<AbstractRational> SubRational>
[[nodiscard]] static auto
json_field_to_abstract_rational(const nlohmann::json &json_row,
                                const char *field_name) {
  if (json_row.contains(field_name)) {
    return SubRational(json_row[field_name]);
  }
  return SubRational();
}

static void add_abstract_rational_to_json(nlohmann::json &json_row,
                                          const AbstractRational &rational,
                                          const char *column_name) {
  if (!rational.is_default()) {
    nlohmann::json json_rational;
    rational.to_json(json_rational);
    json_row[column_name] = std::move(json_rational);
  }
}

struct Rational : public AbstractRational {
  Rational() = default;

  Rational(int numerator, int denominator)
      : AbstractRational(numerator, denominator) {}

  explicit Rational(const nlohmann::json &json_rational)
      : AbstractRational(json_rational) {}

  [[nodiscard]] auto operator==(const Rational &other_rational) const {
    return AbstractRational::operator==(other_rational);
  }
};

Q_DECLARE_METATYPE(Rational);

struct Interval : public AbstractRational {
  int octave = 0;

  Interval() = default;

  Interval(int numerator, int denominator, int octave_input)
      : AbstractRational(numerator, denominator), octave(octave_input) {}

  explicit Interval(const nlohmann::json &json_rational)
      : AbstractRational(json_rational),
        octave(json_rational.value("octave", 0)) {}

  [[nodiscard]] auto operator==(const Interval &other_interval) const {
    return AbstractRational::operator==(other_interval) &&
           octave == other_interval.octave;
  }

  [[nodiscard]] auto is_default() const -> bool override {
    return AbstractRational::is_default() && octave == 0;
  }

  [[nodiscard]] auto to_double() const -> double override {
    return AbstractRational::to_double() * pow(OCTAVE_RATIO, octave);
  }

  void to_json(nlohmann::json &json_interval) const override {
    AbstractRational::to_json(json_interval);
    add_int_to_json(json_interval, "octave", octave, 0);
  }
};

Q_DECLARE_METATYPE(Interval);

static void add_pitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<Instrument>(schema);
  auto interval_fields_schema = get_rational_fields_schema();
  interval_fields_schema["octave"] =
      get_number_schema("integer", -MAX_OCTAVE, MAX_OCTAVE);
  schema["interval"] = get_object_schema(interval_fields_schema);
}

static void add_unpitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<PercussionSet>(schema);
  add_named_schema<PercussionInstrument>(schema);
}

// In addition to the following, a sub-row should have the following methods:
// SubRow(const nlohmann::json& json_row);
// static auto is_column_editable(int column_number) -> bool;
// (optional)
// static auto get_column_name(int column_number) -> QString;
// static auto get_number_of_columns() -> int;
// static auto get_fields_schema() -> nlohmann::json;
// static auto get_plural_field_for() -> const char *;
// void void copy_column_from(const SubRow &template_row, int column_number);
struct Row {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  Row() = default;

  explicit Row(const nlohmann::json &json_chord)
      : beats(json_field_to_abstract_rational<Rational>(json_chord, "beats")),
        velocity_ratio(json_field_to_abstract_rational<Rational>(
            json_chord, "velocity_ratio")),
        words([](const nlohmann::json &json_row) {
          if (json_row.contains("words")) {
            return QString::fromStdString(json_row["words"]);
          }
          return QString("");
        }(json_chord)) {}

  virtual ~Row() = default;

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) {
    return true;
  }

  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;

  virtual void set_data(int column, const QVariant &new_value) = 0;
  virtual void column_to_json(nlohmann::json &json_row,
                              int column_number) const = 0;

  [[nodiscard]] static auto get_fields_schema() {
    return nlohmann::json(
        {{"beats", get_object_schema(get_rational_fields_schema())},
         {"velocity_ratio", get_object_schema(get_rational_fields_schema())},
         {"words", nlohmann::json({{"type", "string"}})}});
  }
};

[[nodiscard]] static auto row_to_json(const Row &row, int left_column,
                                      int right_column) -> nlohmann::json {
  auto json_row = nlohmann::json::object();
  for (auto column_number = left_column; column_number <= right_column;
       column_number++) {
    row.column_to_json(json_row, column_number);
  }
  return json_row;
}

template <std::derived_from<Row> SubRow>
static void partial_json_to_rows(QList<SubRow> &new_rows,
                                 const nlohmann::json &json_rows,
                                 int number_of_rows) {
  std::transform(
      json_rows.cbegin(), json_rows.cbegin() + static_cast<int>(number_of_rows),
      std::back_inserter(new_rows),
      [](const nlohmann::json &json_row) { return SubRow(json_row); });
}

template <std::derived_from<Row> SubRow>
static void json_to_rows(QList<SubRow> &rows, const nlohmann::json &json_rows) {
  partial_json_to_rows(rows, json_rows, static_cast<int>(json_rows.size()));
}

template <std::derived_from<Row> SubRow>
static void add_rows_to_json(nlohmann::json &json_chord,
                             const QList<SubRow> &rows) {
  if (!rows.empty()) {
    nlohmann::json json_rows;
    std::transform(rows.cbegin(), rows.cend(), std::back_inserter(json_rows),
                   [](const SubRow &row) -> nlohmann::json {
                     return row_to_json(row, 0,
                                        row.get_number_of_columns() - 1);
                   });
    json_chord[SubRow::get_plural_field_for()] = std::move(json_rows);
  }
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
json_field_to_rows(const nlohmann::json &json_object) {
  const char *field = SubRow::get_plural_field_for();
  if (json_object.contains(field)) {
    QList<SubRow> rows;
    const auto &json_rows = json_object[field];
    json_to_rows(rows, json_rows);
    return rows;
  }
  return QList<SubRow>();
}

template <std::derived_from<Row> SubRow>
static void add_row_array_schema(nlohmann::json &schema) {
  schema[SubRow::get_plural_field_for()] = nlohmann::json(
      {{"type", "array"},
       {"items", get_object_schema(SubRow::get_fields_schema())}});
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto get_cells_mime() -> const QString & {
  static const auto cells_mime = [] {
    QString cells_mime;
    QTextStream stream(&cells_mime);
    stream << "application/prs." << SubRow::get_plural_field_for()
           << "_cells+json";
    return cells_mime;
  }();
  return cells_mime;
}

[[nodiscard]] static auto get_milliseconds(double beats_per_minute,
                                           const Row &row) {
  return row.beats.to_double() * MILLISECONDS_PER_MINUTE / beats_per_minute;
}

// A subnote should have the following method:
// static auto get_note_type() -> const char*;
struct Note : Row {
  Note() = default;
  explicit Note(const nlohmann::json &json_note) : Row(json_note){};

  [[nodiscard]] virtual auto get_closest_midi(
      QWidget &parent, fluid_sequencer_t *sequencer_pointer,
      fluid_event_t *event_pointer, double current_time, double current_key,
      const PercussionInstrument *current_percussion_instrument_pointer,
      int channel_number, int chord_number, int note_number) const -> short = 0;

  [[nodiscard]] virtual auto
  get_program(QWidget &parent, const Instrument *current_instrument_pointer,
              const PercussionSet *current_percussion_set_pointer,
              int chord_number, int note_number) const -> const Program & = 0;
};

template <std::derived_from<Note> SubNote>
static void add_note_location(QTextStream &stream, int chord_number,
                              int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1
         << QObject::tr(SubNote::get_note_type()) << note_number + 1;
}

template <std::derived_from<Note> SubNote, std::derived_from<Named> SubNamed>
[[nodiscard]] static auto
substitute_named_for(QWidget &parent, const SubNamed *sub_named_pointer,
                     const SubNamed *current_sub_named_pointer,
                     const char *default_one, int chord_number, int note_number,
                     const char *missing_title, const char *missing_message,
                     const char *default_message) -> const SubNamed & {
  if (sub_named_pointer == nullptr) {
    sub_named_pointer = current_sub_named_pointer;
  };
  if (sub_named_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr(missing_message);
    add_note_location<SubNote>(stream, chord_number, note_number);
    stream << QObject::tr(default_message);
    show_warning(parent, missing_title, message);
    sub_named_pointer = &get_by_name<SubNamed>(default_one);
  }
  return *sub_named_pointer;
}

struct UnpitchedNote : Note {
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;

  UnpitchedNote() = default;

  explicit UnpitchedNote(const nlohmann::json &json_note)
      : Note(json_note),
        percussion_set_pointer(
            json_field_to_named_pointer<PercussionSet>(json_note)),
        percussion_instrument_pointer(
            json_field_to_named_pointer<PercussionInstrument>(json_note)) {}

  [[nodiscard]] auto get_closest_midi(
      QWidget &parent, fluid_sequencer_t * /*sequencer_pointer*/,
      fluid_event_t * /*event_pointer*/, double /*current_time*/,
      double /*current_key*/,
      const PercussionInstrument *current_percussion_instrument_pointer,
      int /*channel_number*/, int chord_number,
      int note_number) const -> short override {
    return substitute_named_for<UnpitchedNote>(
               parent, percussion_instrument_pointer,
               current_percussion_instrument_pointer, "Tambourine",
               chord_number, note_number, "Percussion instrument error",
               "No percussion instrument", ". Using Tambourine.")
        .midi_number;
  };

  [[nodiscard]] auto get_program(
      QWidget &parent, const Instrument * /*current_instrument_pointer*/,
      const PercussionSet *current_percussion_set_pointer, int chord_number,
      int note_number) const -> const Program & override {
    return substitute_named_for<UnpitchedNote>(
        parent, percussion_set_pointer, current_percussion_set_pointer,
        "Marimba", chord_number, note_number, "Percussion set error",
        "No percussion set", ". Using Standard.");
  };

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = Row::get_fields_schema();
    add_unpitched_fields_to_schema(schema);
    return schema;
  }

  [[nodiscard]] static auto get_note_type() { return ", unpitched note "; }

  [[nodiscard]] static auto get_plural_field_for() { return "unpitched_notes"; }

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      return "Percussion set";
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
    }
  }

  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_unpitched_note_columns;
  }

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      return QVariant::fromValue(percussion_set_pointer);
    case unpitched_note_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument_pointer);
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

  void set_data(int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      percussion_set_pointer = variant_to<const PercussionSet *>(new_value);
      break;
    case unpitched_note_percussion_instrument_column:
      percussion_instrument_pointer =
          variant_to<const PercussionInstrument *>(new_value);
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

  void copy_column_from(const UnpitchedNote &template_row, int column_number) {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case unpitched_note_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
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

  void column_to_json(nlohmann::json &json_percussion,
                      int column_number) const override {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      add_named_to_json(json_percussion, percussion_set_pointer);
      break;
    case unpitched_note_percussion_instrument_column:
      add_named_to_json(json_percussion, percussion_instrument_pointer);
      break;
    case unpitched_note_beats_column:
      add_abstract_rational_to_json(json_percussion, beats, "beats");
      break;
    case unpitched_note_velocity_ratio_column:
      add_abstract_rational_to_json(json_percussion, velocity_ratio,
                                    "velocity_ratio");
      break;
    case unpitched_note_words_column:
      add_words_to_json(json_percussion, words);
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

  explicit PitchedNote(const nlohmann::json &json_note)
      : Note(json_note),
        instrument_pointer(json_field_to_named_pointer<Instrument>(json_note)),
        interval(
            json_field_to_abstract_rational<Interval>(json_note, "interval")) {}

  [[nodiscard]] auto get_closest_midi(
      QWidget & /*parent*/, fluid_sequencer_t *sequencer_pointer,
      fluid_event_t *event_pointer, double current_time, double current_key,
      const PercussionInstrument * /*current_percussion_instrument_pointer*/,
      int channel_number, int /*chord_number*/,
      int /*note_number*/) const -> short override {
    auto midi_float = get_midi(current_key * interval.to_double());
    auto closest_midi = static_cast<short>(round(midi_float));

    fluid_event_pitch_bend(
        event_pointer, channel_number,
        to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
               BEND_PER_HALFSTEP));
    send_event_at(sequencer_pointer, event_pointer, current_time);
    return closest_midi;
  }

  [[nodiscard]] auto
  get_program(QWidget &parent, const Instrument *current_instrument_pointer,
              const PercussionSet * /*current_percussion_set_pointer*/,
              int chord_number,
              int note_number) const -> const Program & override {
    return substitute_named_for<PitchedNote>(
        parent, instrument_pointer, current_instrument_pointer, "Marimba",
        chord_number, note_number, "Instrument error", "No instrument",
        ". Using Marimba.");
  }

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = Row::get_fields_schema();
    add_pitched_fields_to_schema(schema);
    return schema;
  }

  [[nodiscard]] static auto get_plural_field_for() { return "pitched_notes"; }

  [[nodiscard]] static auto get_note_type() { return ", pitched note "; }

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
    }
  }

  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_pitched_note_columns;
  }

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override {
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

  void set_data(int column_number, const QVariant &new_value) override {
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

  void copy_column_from(const PitchedNote &template_row, int column_number) {
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

  void column_to_json(nlohmann::json &json_note,
                      int column_number) const override {
    switch (column_number) {
    case pitched_note_instrument_column:
      add_named_to_json(json_note, instrument_pointer);
      break;
    case pitched_note_interval_column:
      add_abstract_rational_to_json(json_note, interval, "interval");
      break;
    case pitched_note_beats_column:
      add_abstract_rational_to_json(json_note, beats, "beats");
      break;
    case pitched_note_velocity_ratio_column:
      add_abstract_rational_to_json(json_note, velocity_ratio,
                                    "velocity_ratio");
      break;
    case pitched_note_words_column:
      add_words_to_json(json_note, words);
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }
};

struct Chord : public Row {
  const Instrument *instrument_pointer = nullptr;
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;
  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  Chord() = default;

  explicit Chord(const nlohmann::json &json_chord)
      : Row(json_chord),
        instrument_pointer(json_field_to_named_pointer<Instrument>(json_chord)),
        percussion_set_pointer(
            json_field_to_named_pointer<PercussionSet>(json_chord)),
        percussion_instrument_pointer(
            json_field_to_named_pointer<PercussionInstrument>(json_chord)),
        interval(
            json_field_to_abstract_rational<Interval>(json_chord, "interval")),
        tempo_ratio(json_field_to_abstract_rational<Rational>(json_chord,
                                                              "tempo_ratio")),
        pitched_notes(json_field_to_rows<PitchedNote>(json_chord)),
        unpitched_notes(json_field_to_rows<UnpitchedNote>(json_chord)) {}

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = Row::get_fields_schema();
    add_pitched_fields_to_schema(schema);
    add_unpitched_fields_to_schema(schema);
    schema["tempo_ratio"] = get_object_schema(get_rational_fields_schema());
    add_row_array_schema<PitchedNote>(schema);
    add_row_array_schema<UnpitchedNote>(schema);
    return schema;
  }

  [[nodiscard]] static auto get_plural_field_for() { return "chords"; }

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      return "Instrument";
    case chord_percussion_set_column:
      return "Percussion set";
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
    }
  }

  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_chord_columns;
  }

  [[nodiscard]] static auto is_column_editable(int column_number) {
    return column_number != chord_pitched_notes_column &&
           column_number != chord_unpitched_notes_column;
  }

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override {
    switch (column_number) {
    case chord_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case chord_percussion_set_column:
      return QVariant::fromValue(percussion_set_pointer);
    case chord_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument_pointer);
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

  void set_data(int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = variant_to<const Instrument *>(new_value);
      break;
    case chord_percussion_set_column:
      percussion_set_pointer = variant_to<const PercussionSet *>(new_value);
      break;
    case chord_percussion_instrument_column:
      percussion_instrument_pointer =
          variant_to<const PercussionInstrument *>(new_value);
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

  void copy_column_from(const Chord &template_row, int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case chord_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case chord_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
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

  void column_to_json(nlohmann::json &json_chord,
                      int column_number) const override {
    switch (column_number) {
    case chord_instrument_column:
      add_named_to_json(json_chord, instrument_pointer);
      break;
    case chord_percussion_set_column:
      add_named_to_json(json_chord, percussion_set_pointer);
      break;
    case chord_percussion_instrument_column:
      add_named_to_json(json_chord, percussion_instrument_pointer);
      break;
    case chord_interval_column:
      add_abstract_rational_to_json(json_chord, interval, "interval");
      break;
    case chord_beats_column:
      add_abstract_rational_to_json(json_chord, beats, "beats");
      break;
    case chord_velocity_ratio_column:
      add_abstract_rational_to_json(json_chord, velocity_ratio,
                                    "velocity_ratio");
      break;
    case chord_tempo_ratio_column:
      add_abstract_rational_to_json(json_chord, tempo_ratio, "tempo_ratio");
      break;
    case chord_words_column:
      add_words_to_json(json_chord, words);
      break;
    case chord_pitched_notes_column:
      add_rows_to_json(json_chord, pitched_notes);
      break;
    case chord_unpitched_notes_column:
      add_rows_to_json(json_chord, unpitched_notes);
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  if (mime_type == get_cells_mime<Chord>()) {
    return QObject::tr("chords cells");
  }
  if (mime_type == get_cells_mime<PitchedNote>()) {
    return QObject::tr("pitched notes cells");
  }
  if (mime_type == get_cells_mime<UnpitchedNote>()) {
    return QObject::tr("unpitched notes cells");
  }
  return mime_type;
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto parse_clipboard(QWidget &parent) {
  const auto &mime_data = get_const_reference(get_clipboard().mimeData());
  const auto &mime_type = get_cells_mime<SubRow>();
  if (!mime_data.hasFormat(mime_type)) {
    auto formats = mime_data.formats();
    if (formats.empty()) {
      show_warning(parent, "Empty paste error",
                   QObject::tr("Nothing to paste!"));
      return nlohmann::json();
    };
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Cannot paste ") << get_mime_description(formats[0])
           << QObject::tr(" as ") << get_mime_description(mime_type);
    show_warning(parent, "MIME type error", message);
    return nlohmann::json();
  }
  const auto &copied_text = mime_data.data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_warning(parent, "Parsing error", parse_error.what());
    return nlohmann::json();
  }
  if (copied.empty()) {
    show_warning(parent, "Empty paste", QObject::tr("Nothing to paste!"));
    return nlohmann::json();
  }
  static const auto cells_validator = []() {
    auto last_column = SubRow::get_number_of_columns() - 1;
    nlohmann::json cells_schema(
        {{"left_column", get_number_schema("integer", 0, last_column)},
         {"right_column", get_number_schema("integer", 0, last_column)}});
    add_row_array_schema<SubRow>(cells_schema);
    return make_validator(nlohmann::json({"left_column", "right_column",
                                          SubRow::get_plural_field_for()}),
                          cells_schema);
  }();

  try {
    cells_validator.validate(copied);
  } catch (const std::exception &error) {
    show_warning(parent, "Schema error", error.what());
    return nlohmann::json();
  }
  return copied;
}

struct Song {
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
};

[[nodiscard]] static auto get_key_text(const Song &song, int chord_number,
                                       double ratio = 1) {
  const auto &chords = song.chords;
  auto key = song.starting_key;
  for (auto previous_chord_number = 0; previous_chord_number <= chord_number;
       previous_chord_number++) {
    key = key * chords.at(previous_chord_number).interval.to_double();
  }
  key = key * ratio;
  auto midi_float = get_midi(key);
  auto closest_midi = to_int(midi_float);
  auto difference_from_c = closest_midi - C_0_MIDI;
  auto octave =
      difference_from_c / HALFSTEPS_PER_OCTAVE; // floor integer division
  auto degree = difference_from_c - octave * HALFSTEPS_PER_OCTAVE;
  auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  QString result;
  QTextStream stream(&result);
  stream << key << QObject::tr(" Hz; ") << QObject::tr([](int degree) {
    switch (degree) {
    case c_degree:
      return "C";
    case c_sharp_degree:
      return "C♯";
    case d_degree:
      return "D";
    case e_flat_degree:
      return "E♭";
    case e_degree:
      return "E";
    case f_degree:
      return "F";
    case f_sharp_degree:
      return "F♯";
    case g_degree:
      return "G";
    case a_flat_degree:
      return "A♭";
    case a_degree:
      return "A";
    case b_flat_degree:
      return "B♭";
    case b_degree:
      return "B";
    default:
      Q_ASSERT(false);
      return "";
    }
  }(degree))
         << octave;
  if (cents != 0) {
    stream << QObject::tr(cents >= 0 ? " + " : " − ") << abs(cents)
           << QObject::tr(" cents");
  }
  return result;
}

static auto
make_audio_driver(QWidget &parent, fluid_settings_t *settings_pointer,
                  fluid_synth_t *synth_pointer) -> fluid_audio_driver_t * {
#ifndef NO_REALTIME_AUDIO
  auto *audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
  if (audio_driver_pointer == nullptr) {
    show_warning(parent, "Audio driver error",
                 QObject::tr("Cannot start audio driver"));
  }
  return audio_driver_pointer;
#else
  return nullptr;
#endif
}

static void delete_audio_driver(fluid_audio_driver_t *audio_driver_pointer) {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }
}

static void stop_playing(fluid_event_t *event_pointer,
                         fluid_sequencer_t *sequencer_pointer) {
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

struct Player {
  // data
  QWidget &parent;

  // play state fields
  QList<double> channel_schedules = QList<double>(NUMBER_OF_MIDI_CHANNELS, 0);

  const Instrument *current_instrument_pointer = nullptr;
  const PercussionSet *current_percussion_set_pointer = nullptr;
  const PercussionInstrument *current_percussion_instrument_pointer = nullptr;

  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

  fluid_settings_t *const settings_pointer = []() {
    fluid_settings_t *settings_pointer = new_fluid_settings();
    Q_ASSERT(settings_pointer != nullptr);
    auto cores = std::thread::hardware_concurrency();
    if (cores > 0) {
      set_fluid_int(settings_pointer, "synth.cpu-cores",
                    static_cast<int>(cores));
    }
#ifdef __linux__
    fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#endif
    return settings_pointer;
  }();
  fluid_event_t *const event_pointer = new_fluid_event();
  fluid_sequencer_t *const sequencer_pointer = new_fluid_sequencer2(0);
  fluid_synth_t *synth_pointer = new_fluid_synth(settings_pointer);
  const unsigned int soundfont_id = get_soundfont_id(synth_pointer);
  const fluid_seq_id_t sequencer_id =
      fluid_sequencer_register_fluidsynth(sequencer_pointer, synth_pointer);

  fluid_audio_driver_t *audio_driver_pointer =
      make_audio_driver(parent, settings_pointer, synth_pointer);

  // methods
  explicit Player(QWidget &parent_input) : parent(parent_input) {
    fluid_event_set_dest(event_pointer, sequencer_id);
  }

  ~Player() {
    stop_playing(event_pointer, sequencer_pointer);
    delete_audio_driver(audio_driver_pointer);
    fluid_sequencer_unregister_client(sequencer_pointer, sequencer_id);
    delete_fluid_sequencer(sequencer_pointer);
    delete_fluid_event(event_pointer);
    delete_fluid_synth(synth_pointer);
    delete_fluid_settings(settings_pointer);
  }

  // prevent moving and copying
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};

[[nodiscard]] static auto player_get_gain(const Player &player) {
  return fluid_synth_get_gain(player.synth_pointer);
}

static void initialize_play(Player &player, const Song &song) {
  player.current_instrument_pointer = nullptr;
  player.current_percussion_set_pointer = nullptr;
  player.current_percussion_instrument_pointer = nullptr;
  player.current_key = song.starting_key;
  player.current_velocity = song.starting_velocity;
  player.current_tempo = song.starting_tempo;
  player.current_time = fluid_sequencer_get_tick(player.sequencer_pointer);

  auto &channel_schedules = player.channel_schedules;
  for (auto index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

static void modulate(Player &player, const Chord &chord) {
  player.current_key = player.current_key * chord.interval.to_double();
  player.current_velocity =
      player.current_velocity * chord.velocity_ratio.to_double();
  player.current_tempo = player.current_tempo * chord.tempo_ratio.to_double();
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    player.current_instrument_pointer = chord_instrument_pointer;
  }

  const auto *chord_percussion_set_pointer = chord.percussion_set_pointer;
  if (chord_percussion_set_pointer != nullptr) {
    player.current_percussion_set_pointer = chord_percussion_set_pointer;
  }

  const auto *chord_percussion_instrument_pointer =
      chord.percussion_instrument_pointer;
  if (chord_percussion_instrument_pointer != nullptr) {
    player.current_percussion_instrument_pointer =
        chord_percussion_instrument_pointer;
  }
}

static void modulate_before_chord(Player &player, const Song &song,
                                  int next_chord_number) {
  const auto &chords = song.chords;
  if (next_chord_number > 0) {
    for (auto chord_number = 0; chord_number < next_chord_number;
         chord_number = chord_number + 1) {
      modulate(player, chords.at(chord_number));
    }
  }
}

static void update_final_time(Player &player, double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
}

template <std::derived_from<Note> SubNote>
static void play_notes(Player &player, int chord_number,
                       const QList<SubNote> &sub_notes, int first_note_number,
                       int number_of_notes) {
  auto &parent = player.parent;
  auto *sequencer_pointer = player.sequencer_pointer;
  auto *event_pointer = player.event_pointer;
  auto &channel_schedules = player.channel_schedules;
  const auto soundfont_id = player.soundfont_id;

  const auto current_time = player.current_time;
  const auto current_velocity = player.current_velocity;
  const auto current_tempo = player.current_tempo;
  const auto current_key = player.current_key;
  const auto *current_percussion_instrument_pointer =
      player.current_percussion_instrument_pointer;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    auto channel_number = -1;
    for (auto channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
         channel_index = channel_index + 1) {
      if (current_time >= channel_schedules.at(channel_index)) {
        channel_number = channel_index;
      }
    }
    if (channel_number == -1) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Out of MIDI channels");
      add_note_location<SubNote>(stream, chord_number, note_number);
      stream << QObject::tr(". Not playing note.");
      show_warning(parent, "MIDI channel error", message);
      return;
    }
    const auto &sub_note = sub_notes.at(note_number);

    const Program &program = sub_note.get_program(
        player.parent, player.current_instrument_pointer,
        player.current_percussion_set_pointer, chord_number, note_number);

    fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                               program.bank_number, program.preset_number);
    send_event_at(sequencer_pointer, event_pointer, current_time);

    auto midi_number = sub_note.get_closest_midi(
        parent, sequencer_pointer, event_pointer, current_time, current_key,
        current_percussion_instrument_pointer, channel_number, chord_number,
        note_number);

    auto velocity = current_velocity * sub_note.velocity_ratio.to_double();
    short new_velocity = 1;
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location<SubNote>(stream, chord_number, note_number);
      stream << QObject::tr(". Playing with velocity ") << MAX_VELOCITY;
      show_warning(parent, "Velocity error", message);
    } else {
      new_velocity = static_cast<short>(std::round(velocity));
    }
    fluid_event_noteon(event_pointer, channel_number, midi_number,
                       new_velocity);
    send_event_at(sequencer_pointer, event_pointer, current_time);

    auto end_time = current_time + get_milliseconds(current_tempo, sub_note);

    fluid_event_noteoff(event_pointer, channel_number, midi_number);
    send_event_at(sequencer_pointer, event_pointer, end_time);

    channel_schedules[channel_number] = end_time;
  }
}

template <std::derived_from<Note> SubNote>
static void play_all_notes(Player &player, int chord_number,
                           const QList<SubNote> &sub_notes) {
  play_notes(player, chord_number, sub_notes, 0,
             static_cast<int>(sub_notes.size()));
}

static void play_chords(Player &player, const Song &song,
                        int first_chord_number, int number_of_chords,
                        int wait_frames = 0) {
  auto start_time = player.current_time + wait_frames;
  player.current_time = start_time;
  update_final_time(player, start_time);
  const auto &chords = song.chords;
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);

    modulate(player, chord);
    play_all_notes(player, chord_number, chord.pitched_notes);
    play_all_notes(player, chord_number, chord.unpitched_notes);
    auto new_current_time =
        player.current_time + get_milliseconds(player.current_tempo, chord);
    player.current_time = new_current_time;
    update_final_time(player, new_current_time);
  }
}

static void export_song_to_file(Player &player, const Song &song,
                                const QString &output_file) {
  auto *settings_pointer = player.settings_pointer;
  auto *event_pointer = player.event_pointer;
  auto *sequencer_pointer = player.sequencer_pointer;

  stop_playing(event_pointer, sequencer_pointer);
  delete_audio_driver(player.audio_driver_pointer);
  check_fluid_ok(fluid_settings_setstr(settings_pointer, "audio.file.name",
                                       output_file.toStdString().c_str()));

  set_fluid_int(settings_pointer, "synth.lock-memory", 0);

  auto finished = false;
  auto finished_timer_id = fluid_sequencer_register_client(
      player.sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(player, song);
  play_chords(player, song, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  fluid_event_set_dest(event_pointer, finished_timer_id);
  fluid_event_timer(event_pointer, nullptr);
  send_event_at(sequencer_pointer, event_pointer,
                player.final_time + START_END_MILLISECONDS);

  auto *renderer_pointer = new_fluid_file_renderer(player.synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    auto process_result = fluid_file_renderer_process_block(renderer_pointer);
    Q_ASSERT(process_result == FLUID_OK);
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(event_pointer, player.sequencer_id);
  set_fluid_int(settings_pointer, "synth.lock-memory", 1);
  player.audio_driver_pointer = make_audio_driver(
      player.parent, player.settings_pointer, player.synth_pointer);
}

static void set_starting_double(Song &song, Player &player,
                                ControlId control_id, QDoubleSpinBox &spin_box,
                                double set_value) {
  switch (control_id) {
  case gain_id:
    fluid_synth_set_gain(player.synth_pointer, static_cast<float>(set_value));
    break;
  case starting_key_id:
    song.starting_key = set_value;
    break;
  case starting_velocity_id:
    song.starting_velocity = set_value;
    break;
  case starting_tempo_id:
    song.starting_tempo = set_value;
  }
  const QSignalBlocker blocker(spin_box);
  spin_box.setValue(set_value);
}

struct SetStartingDouble : public QUndoCommand {
  Song &song;
  Player &player;
  QDoubleSpinBox &spin_box;
  const ControlId control_id;
  const double old_value;
  double new_value;

  explicit SetStartingDouble(Song &song_input, Player &player_input,
                             QDoubleSpinBox &spin_box_input,
                             ControlId command_id_input, double old_value_input,
                             double new_value_input)
      : song(song_input), player(player_input), spin_box(spin_box_input),
        control_id(command_id_input), old_value(old_value_input),
        new_value(new_value_input) {}

  [[nodiscard]] auto id() const -> int override { return control_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);

    const auto *next_velocity_change_pointer =
        dynamic_cast<const SetStartingDouble *>(next_command_pointer);

    new_value = get_const_reference(next_velocity_change_pointer).new_value;
    return true;
  }

  void undo() override {
    set_starting_double(song, player, control_id, spin_box, old_value);
  }

  void redo() override {
    set_starting_double(song, player, control_id, spin_box, new_value);
  }
};

static void set_double(QUndoStack &undo_stack, Song &song, Player &player,
                       QDoubleSpinBox &spin_box, ControlId control_id,
                       double old_value, double new_value) {
  undo_stack.push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          song, player, spin_box, control_id, old_value, new_value));
}

template <std::derived_from<Row> SubRow> struct SetCell;

template <std::derived_from<Row> SubRow>
struct RowsModel : public QAbstractTableModel {
  QList<SubRow> *rows_pointer = nullptr;
  QUndoStack &undo_stack;

  explicit RowsModel(QUndoStack &undo_stack_input)
      : undo_stack(undo_stack_input){};

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    return static_cast<int>(get_const_reference(rows_pointer).size());
  }

  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override {
    return SubRow::get_number_of_columns();
  }

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return RowsModel::tr(SubRow::get_column_name(section));
    case Qt::Vertical:
      return section + 1;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (SubRow::is_column_editable(index.column())) {
      return uneditable | Qt::ItemIsEditable;
    }
    return uneditable;
  }

  [[nodiscard]] virtual auto get_status(int /*row_number*/) const -> QString {
    return "";
  }

  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override {
    Q_ASSERT(index.isValid());
    auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      return get_status(row_number);
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
      return {};
    }

    return get_const_reference(rows_pointer)
        .at(row_number)
        .get_data(index.column());
  }

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override {
    // only set data for edit
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack.push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            *this, index, new_value));
    return true;
  }

  // don't inline these functions because they use protected methods
  void set_cell(const QModelIndex &set_index, const QVariant &new_value) {
    auto row_number = set_index.row();
    auto column_number = set_index.column();

    get_reference(rows_pointer)[row_number].set_data(column_number, new_value);
    dataChanged(index(row_number, column_number),
                index(row_number, column_number),
                {Qt::DisplayRole, Qt::EditRole});
  }

  void set_cells(int first_row_number, const QList<SubRow> &new_rows,
                 int left_column, int right_column) {
    auto &rows = get_reference(rows_pointer);
    auto number_of_new_rows = new_rows.size();
    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const auto &new_row = new_rows.at(replace_number);
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(new_row, column_number);
      }
    }
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_new_rows - 1, right_column),
                {Qt::DisplayRole, Qt::EditRole});
  }

  void delete_cells(int first_row_number, int number_of_rows, int left_column,
                    int right_column) {
    auto &rows = get_reference(rows_pointer);
    for (auto replace_number = 0; replace_number < number_of_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const SubRow empty_row;
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(empty_row, column_number);
      }
    }
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_rows - 1, right_column),
                {Qt::DisplayRole, Qt::EditRole});
  }

  void insert_json_rows(int first_row_number, const nlohmann::json &json_rows) {
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + static_cast<int>(json_rows.size()) - 1);
    json_to_rows(get_reference(rows_pointer), json_rows);
    endInsertRows();
  }

  void insert_rows(int first_row_number, const QList<SubRow> &new_rows) {
    auto &rows = get_reference(rows_pointer);
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + new_rows.size() - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
  }

  void insert_row(int row_number, const SubRow &new_row) {
    beginInsertRows(QModelIndex(), row_number, row_number);
    auto &rows = get_reference(rows_pointer);
    rows.insert(rows.begin() + row_number, new_row);
    endInsertRows();
  }

  void remove_rows(int first_row_number, int number_of_rows) {
    auto &rows = get_reference(rows_pointer);
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }

  void set_rows_pointer(QList<SubRow> *new_rows_pointer = nullptr) {
    beginResetModel();
    rows_pointer = new_rows_pointer;
    endResetModel();
  }
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
get_rows(RowsModel<SubRow> &rows_model) -> QList<SubRow> & {
  return get_reference(rows_model.rows_pointer);
}

template <std::derived_from<Row> SubRow>
static void copy_template(QMimeData &mime_data,
                          const RowsModel<SubRow> &rows_model,
                          int first_row_number, int number_of_rows,
                          int left_column, int right_column) {
  const auto &rows = get_const_reference(rows_model.rows_pointer);
  nlohmann::json copied_json = nlohmann::json::array();
  std::transform(
      rows.cbegin() + first_row_number,
      rows.cbegin() + first_row_number + number_of_rows,
      std::back_inserter(copied_json),
      [left_column, right_column](const SubRow &row) -> nlohmann::json {
        return row_to_json(row, left_column, right_column);
      });

  const nlohmann::json copied(
      {{"left_column", left_column},
       {"right_column", right_column},
       {SubRow::get_plural_field_for(), std::move(copied_json)}});

  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  mime_data.setData(get_cells_mime<SubRow>(), json_text.str().c_str());
}

template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
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

template <std::derived_from<Row> SubRow>
struct DeleteCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;

  explicit DeleteCells(RowsModel<SubRow> &rows_model_input,
                       int first_row_number_input, int number_of_rows,
                       int left_column_input, int right_column_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        left_column(left_column_input), right_column(right_column_input),
        old_rows(copy_items(get_rows(rows_model), first_row_number,
                            number_of_rows)) {}

  void undo() override {
    rows_model.set_cells(first_row_number, old_rows, left_column, right_column);
  }

  void redo() override {
    rows_model.delete_cells(first_row_number, old_rows.size(), left_column,
                            right_column);
  }
};

template <std::derived_from<Row> SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;

  explicit SetCells(RowsModel<SubRow> &rows_model_input,
                    int first_row_number_input,
                    const nlohmann::json &json_cells,
                    QList<SubRow> old_rows_input, QList<SubRow> new_rows_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        left_column(get_json_int(json_cells, "left_column")),
        right_column(get_json_int(json_cells, "right_column")),
        old_rows(std::move(old_rows_input)),
        new_rows(std::move(new_rows_input)) {}

  void undo() override {
    rows_model.set_cells(first_row_number, old_rows, left_column, right_column);
  }

  void redo() override {
    rows_model.set_cells(first_row_number, new_rows, left_column, right_column);
  }
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
paste_cells_template(QWidget &parent, int first_row_number,
                     RowsModel<SubRow> &rows_model) -> QUndoCommand * {
  const auto json_cells = parse_clipboard<SubRow>(parent);
  if (json_cells.empty()) {
    return nullptr;
  }
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  const auto &rows = get_rows(rows_model);

  auto number_of_rows =
      std::min({static_cast<int>(json_rows.size()),
                static_cast<int>(rows.size()) - first_row_number});

  QList<SubRow> new_rows;
  partial_json_to_rows(new_rows, json_rows, number_of_rows);

  return new SetCells( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number, json_cells,
      copy_items(rows, first_row_number, number_of_rows), std::move(new_rows));
}

template <std::derived_from<Row> SubRow>
struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow> &rows_model_input, int row_number_input,
            SubRow new_row_input = SubRow())
      : rows_model(rows_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)) {}

  void undo() override { rows_model.remove_rows(row_number, 1); }

  void redo() override { rows_model.insert_row(row_number, new_row); }
};

template <std::derived_from<Row> SubRow>
static void
insert_or_remove(RowsModel<SubRow> &rows_model, int first_row_number,
                 const QList<SubRow> &new_rows, bool should_insert) {
  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}

template <std::derived_from<Row> SubRow>
struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> &rows_model_input,
                   int first_row_number_input, QList<SubRow> new_rows_input,
                   bool backwards_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), backwards(backwards_input) {}

  void undo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, backwards);
  }

  void redo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, !backwards);
  }
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
paste_insert_template(QWidget &parent, RowsModel<SubRow> &rows_model,
                      int row_number) -> QUndoCommand * {
  const auto json_cells = parse_clipboard<SubRow>(parent);
  if (json_cells.empty()) {
    return nullptr;
  }
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  QList<SubRow> new_rows;
  json_to_rows(new_rows, json_rows);
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, std::move(new_rows), false);
}

template <std::derived_from<Row> SubRow>
static auto remove_rows_template(RowsModel<SubRow> &rows_model,
                                 int first_row_number,
                                 int number_of_rows) -> QUndoCommand * {
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(get_rows(rows_model), first_row_number, number_of_rows), true);
}

struct ChordsModel : public RowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input)
      : RowsModel(undo_stack), song(song_input) {
    rows_pointer = &song.chords;
  }

  [[nodiscard]] auto get_status(int row_number) const -> QString override {
    return get_key_text(song, row_number);
  }
};

struct PitchedNotesModel : public RowsModel<PitchedNote> {
  Song &song;
  int parent_chord_number = -1;

  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song_input)
      : RowsModel<PitchedNote>(undo_stack), song(song_input) {}

  [[nodiscard]] auto get_status(int row_number) const -> QString override {
    return get_key_text(
        song, parent_chord_number,
        get_const_reference(rows_pointer).at(row_number).interval.to_double());
  }
};

struct FileMenu : public QMenu {
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString current_file;

  QAction save_action = make_action("&Save");
  QAction open_action = make_action("&Open");
  QAction save_as_action = make_action("&Save As...");
  QAction export_action = make_action("&Export recording");

  FileMenu(const char *name, QWidget &parent)
      : QMenu(QObject::tr(name), &parent) {
    add_menu_action(*this, open_action, QKeySequence::Open);
    addSeparator();
    add_menu_action(*this, save_action, QKeySequence::Save, false);
    add_menu_action(*this, save_as_action, QKeySequence::SaveAs);
    add_menu_action(*this, export_action);
  }
};

static void save_song_as_file(QUndoStack &undo_stack, FileMenu &file_menu,
                              const Song &song, Player &player,
                              const QString &filename) {
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = player_get_gain(player);
  json_song["starting_key"] = song.starting_key;
  json_song["starting_tempo"] = song.starting_tempo;
  json_song["starting_velocity"] = song.starting_velocity;

  add_rows_to_json(json_song, song.chords);

  file_io << std::setw(4) << json_song;
  file_io.close();
  file_menu.current_file = filename;

  undo_stack.setClean();
}

[[nodiscard]] static auto get_selected_file(FileMenu &file_menu,
                                            const QFileDialog &dialog) {
  file_menu.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

struct PasteMenu : public QMenu {
  QAction paste_over_action = make_action("&Over");
  QAction paste_into_action = make_action("&Into start");
  QAction paste_after_action = make_action("&After");
  PasteMenu(const char *name, QWidget &parent)
      : QMenu(QObject::tr(name), &parent) {
    add_menu_action(*this, paste_over_action, QKeySequence::Paste, false);
    add_menu_action(*this, paste_into_action);
    add_menu_action(*this, paste_after_action, QKeySequence::StandardKey(),
                    false);
  }
};

struct InsertMenu : public QMenu {
  QAction insert_after_action = make_action("&After");
  QAction insert_into_action = make_action("&Into start");

  InsertMenu(const char *name, QWidget &parent)
      : QMenu(QObject::tr(name), &parent) {
    add_menu_action(*this, insert_after_action,
                    QKeySequence::InsertLineSeparator, false);
    add_menu_action(*this, insert_into_action, QKeySequence::AddTab);
  }
};

struct EditMenu : public QMenu {
  QAction cut_action = make_action("&Cut");
  QAction copy_action = make_action("&Copy");
  PasteMenu &paste_menu = *(new PasteMenu("&Paste", *this));
  InsertMenu &insert_menu = *(new InsertMenu("&Insert", *this));
  QAction delete_cells_action = make_action("&Delete cells");
  QAction remove_rows_action = make_action("&Remove rows");
  QAction back_to_chords_action = make_action("&Back to chords");

  EditMenu(const char *name, QWidget &parent, QUndoStack &undo_stack)
      : QMenu(QObject::tr(name), &parent) {

    auto &undo_action = get_reference(undo_stack.createUndoAction(this));
    undo_action.setShortcuts(QKeySequence::Undo);
    addAction(&undo_action);

    auto &redo_action = get_reference(undo_stack.createRedoAction(this));
    redo_action.setShortcuts(QKeySequence::Redo);
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

    add_menu_action(*this, back_to_chords_action, QKeySequence::Back, false);
  }
};

struct PlayMenu : public QMenu {
  QAction play_action = make_action("&Play selection");
  QAction stop_playing_action = make_action("&Stop playing");

  PlayMenu(const char *name, QWidget &parent)
      : QMenu(QObject::tr(name), &parent) {
    add_menu_action(*this, play_action, QKeySequence::Print, false);
    add_menu_action(*this, stop_playing_action, QKeySequence::Cancel);
  }
};

struct SwitchTable : public QWidget {
  ModelType current_model_type = chords_type;
  int current_chord_number = -1;

  QLabel &editing_text = *(new QLabel(QObject::tr("Editing chords")));
  QTableView &table_view = *(new QTableView);
  ChordsModel chords_model;
  PitchedNotesModel pitched_notes_model;
  RowsModel<UnpitchedNote> unpitched_notes_model;

  SwitchTable(QWidget &parent, QUndoStack &undo_stack, Song &song)
      : QWidget(&parent), chords_model(ChordsModel(undo_stack, song)),
        pitched_notes_model(PitchedNotesModel(undo_stack, song)),
        unpitched_notes_model(RowsModel<UnpitchedNote>(undo_stack)) {
    table_view.setSelectionMode(QAbstractItemView::ContiguousSelection);
    table_view.setSelectionBehavior(QAbstractItemView::SelectItems);
    table_view.setSizeAdjustPolicy(
        QAbstractScrollArea::AdjustToContentsOnFirstShow);

    get_reference(table_view.horizontalHeader())
        .setSectionResizeMode(QHeaderView::ResizeToContents);

    table_view.setMouseTracking(true);

    auto &table_column_layout = // NOLINT(cppcoreguidelines-owning-memory)
        *(new QVBoxLayout(this));
    table_column_layout.addWidget(&editing_text);
    table_column_layout.addWidget(&table_view);
  }
};

[[nodiscard]] static auto
get_selection_model(const SwitchTable &switch_table) -> QItemSelectionModel & {
  return get_reference(switch_table.table_view.selectionModel());
}

[[nodiscard]] static auto get_selection(const SwitchTable &switch_table) {
  return get_selection_model(switch_table).selection();
}

[[nodiscard]] static auto get_only_range(const SwitchTable &switch_table) {
  return get_only(get_selection(switch_table));
}

[[nodiscard]] static auto get_next_row(const SwitchTable &switch_table) {
  return get_only(get_selection(switch_table)).bottom() + 1;
}

static void double_click_chord_column(SwitchTable &switch_table,
                                      int chord_number, int chord_column) {
  switch_table.table_view.doubleClicked(
      switch_table.chords_model.index(chord_number, chord_column));
}

static void update_actions(const SwitchTable &switch_table, EditMenu &edit_menu,
                           PlayMenu &play_menu) {
  auto anything_selected = !get_selection(switch_table).empty();

  edit_menu.cut_action.setEnabled(anything_selected);
  edit_menu.copy_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_over_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_after_action.setEnabled(anything_selected);
  edit_menu.insert_menu.insert_after_action.setEnabled(anything_selected);
  edit_menu.delete_cells_action.setEnabled(anything_selected);
  edit_menu.remove_rows_action.setEnabled(anything_selected);
  play_menu.play_action.setEnabled(anything_selected);
}

static void copy_selection(const SwitchTable &switch_table) {
  const auto &range = get_only_range(switch_table);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);
  auto left_column = range.left();
  auto right_column = range.right();
  auto &mime_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  switch (switch_table.current_model_type) {
  case chords_type:
    copy_template(mime_data, switch_table.chords_model, first_row_number,
                  number_of_rows, left_column, right_column);
    break;
  case pitched_notes_type:
    copy_template(mime_data, switch_table.pitched_notes_model, first_row_number,
                  number_of_rows, left_column, right_column);
    break;
  case unpitched_notes_type:
    copy_template(mime_data, switch_table.unpitched_notes_model,
                  first_row_number, number_of_rows, left_column, right_column);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  get_clipboard().setMimeData(&mime_data);
}

static void set_model(SwitchTable &switch_table, EditMenu &edit_menu,
                      PlayMenu &play_menu, QAbstractItemModel &model) {
  switch_table.table_view.setModel(&model);
  update_actions(switch_table, edit_menu, play_menu);
  QObject::connect(&get_selection_model(switch_table),
                   &QItemSelectionModel::selectionChanged, &switch_table,
                   [&switch_table, &edit_menu, &play_menu]() {
                     update_actions(switch_table, edit_menu, play_menu);
                   });
}

static void connect_model(const SwitchTable &switch_table, EditMenu &edit_menu,
                          PlayMenu &play_menu,
                          const QAbstractItemModel &model) {
  QObject::connect(&model, &QAbstractItemModel::rowsInserted, &switch_table,
                   [&switch_table, &edit_menu, &play_menu]() {
                     update_actions(switch_table, edit_menu, play_menu);
                   });
  QObject::connect(&model, &QAbstractItemModel::rowsRemoved, &switch_table,
                   [&switch_table, &edit_menu, &play_menu]() {
                     update_actions(switch_table, edit_menu, play_menu);
                   });
}

static void delete_cells(QUndoStack &undo_stack, SwitchTable &switch_table) {
  const auto &range = get_only_range(switch_table);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);
  auto left_column = range.left();
  auto right_column = range.right();

  QUndoCommand *undo_command = nullptr;
  switch (switch_table.current_model_type) {
  case chords_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.chords_model, first_row_number, number_of_rows,
        left_column, right_column);
    break;
  case pitched_notes_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.pitched_notes_model, first_row_number, number_of_rows,
        left_column, right_column);
    break;
  case unpitched_notes_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.unpitched_notes_model, first_row_number, number_of_rows,
        left_column, right_column);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  undo_stack.push(undo_command);
}

static void paste_insert(QUndoStack &undo_stack, SwitchTable &switch_table,
                         int row_number) {
  QUndoCommand *undo_command = nullptr;
  switch (switch_table.current_model_type) {
  case chords_type:
    undo_command = paste_insert_template(switch_table,
                                         switch_table.chords_model, row_number);
    break;
  case pitched_notes_type:
    undo_command = paste_insert_template(
        switch_table, switch_table.pitched_notes_model, row_number);
    break;
  case unpitched_notes_type:
    undo_command = paste_insert_template(
        switch_table, switch_table.unpitched_notes_model, row_number);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  if (undo_command == nullptr) {
    return;
  }
  undo_stack.push(undo_command);
}

static void insert_model_row(QUndoStack &undo_stack, SwitchTable &switch_table,
                             Song &song, int row_number) {
  const auto current_model_type = switch_table.current_model_type;
  QUndoCommand *undo_command = nullptr;
  if (current_model_type == chords_type) {
    undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.chords_model, row_number);
  } else {
    const auto &chord_beats =
        song.chords[switch_table.current_chord_number].beats;
    if (current_model_type == pitched_notes_type) {
      PitchedNote new_pitched_note;
      new_pitched_note.beats = chord_beats;
      undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
          switch_table.pitched_notes_model, row_number,
          std::move(new_pitched_note));
    } else {
      UnpitchedNote new_unpitched_note;
      new_unpitched_note.beats = chord_beats;
      undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
          switch_table.unpitched_notes_model, row_number,
          std::move(new_unpitched_note));
    }
  }
  undo_stack.push(undo_command);
}

static void edit_children_or_back(SwitchTable &switch_table,
                                  FileMenu &file_menu, EditMenu &edit_menu,
                                  PlayMenu &play_menu, int chord_number,
                                  bool is_pitched, bool should_edit_children) {
  auto &editing_text = switch_table.editing_text;

  edit_menu.back_to_chords_action.setEnabled(should_edit_children);
  file_menu.open_action.setEnabled(!should_edit_children);

  if (should_edit_children) {
    QString label_text;
    QTextStream stream(&label_text);
    stream << QObject::tr(is_pitched ? "Editing pitched notes for chord "
                                     : "Editing unpitched notes for chord ")
           << chord_number + 1;
    editing_text.setText(label_text);
    Q_ASSERT(switch_table.current_model_type == chords_type);
    switch_table.current_model_type =
        is_pitched ? pitched_notes_type : unpitched_notes_type;
    switch_table.current_chord_number = chord_number;

    auto &chord = get_rows(switch_table.chords_model)[chord_number];
    if (is_pitched) {
      auto &pitched_notes_model = switch_table.pitched_notes_model;
      pitched_notes_model.parent_chord_number = chord_number;
      pitched_notes_model.set_rows_pointer(&chord.pitched_notes);
      set_model(switch_table, edit_menu, play_menu, pitched_notes_model);
    } else {
      auto &unpitched_notes_model = switch_table.unpitched_notes_model;
      unpitched_notes_model.set_rows_pointer(&chord.unpitched_notes);
      set_model(switch_table, edit_menu, play_menu, unpitched_notes_model);
    }
  } else {
    set_model(switch_table, edit_menu, play_menu, switch_table.chords_model);

    editing_text.setText(QObject::tr("Editing chords"));
    switch_table.current_model_type = chords_type;
    switch_table.current_chord_number = -1;

    if (is_pitched) {
      auto &pitched_notes_model = switch_table.pitched_notes_model;
      pitched_notes_model.set_rows_pointer();
      pitched_notes_model.parent_chord_number = -1;
    } else {
      switch_table.unpitched_notes_model.set_rows_pointer();
    }
  }
}

struct EditChildrenOrBack : public QUndoCommand {
  SwitchTable &switch_table;
  FileMenu &file_menu;
  EditMenu &edit_menu;
  PlayMenu &play_menu;
  int chord_number;
  bool is_pitched;
  bool backwards;

  explicit EditChildrenOrBack(SwitchTable &switch_table_input,
                              FileMenu &file_menu_input,
                              EditMenu &edit_menu_input,
                              PlayMenu &play_menu_input, int chord_number_input,
                              bool is_notes_input, bool backwards_input)
      : switch_table(switch_table_input), file_menu(file_menu_input),
        edit_menu(edit_menu_input), play_menu(play_menu_input),
        chord_number(chord_number_input), is_pitched(is_notes_input),
        backwards(backwards_input) {}

  void undo() override {
    edit_children_or_back(switch_table, file_menu, edit_menu, play_menu,
                          chord_number, is_pitched, backwards);
  }

  void redo() override {
    edit_children_or_back(switch_table, file_menu, edit_menu, play_menu,
                          chord_number, is_pitched, !backwards);
  }
};

static void add_edit_children_or_back(QUndoStack &undo_stack,
                                      SwitchTable &switch_table,
                                      FileMenu &file_menu, EditMenu &edit_menu,
                                      PlayMenu &play_menu, int chord_number,
                                      bool is_pitched, bool backwards) {
  undo_stack.push(
      new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
          switch_table, file_menu, edit_menu, play_menu, chord_number,
          is_pitched, backwards));
}

struct Controls : public QWidget {
  QDoubleSpinBox &gain_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_key_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_velocity_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_tempo_editor = *(new QDoubleSpinBox);
  explicit Controls() {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto &controls_form = // NOLINT(cppcoreguidelines-owning-memory)
        *(new QFormLayout(this));

    add_control(controls_form, "&Gain:", gain_editor, 0, MAX_GAIN, "/10",
                GAIN_STEP, 1);

    add_control(controls_form, "Starting &key:", starting_key_editor,
                MIN_STARTING_KEY, MAX_STARTING_KEY, " hz");
    add_control(controls_form, "Starting &velocity:", starting_velocity_editor,
                0, MAX_VELOCITY, "/127");

    add_control(controls_form, "Starting &tempo:", starting_tempo_editor,
                MIN_STARTING_TEMPO, MAX_STARTING_TEMPO, " bpm");
  }
};

static void open_file_split(QUndoStack &undo_stack, FileMenu &file_menu,
                            Controls &controls, SwitchTable &switch_table,
                            const QString &filename) {
  auto &chords_model = switch_table.chords_model;
  auto number_of_chords = chords_model.rowCount(QModelIndex());
  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_warning(file_menu, "Parsing error", parse_error.what());
    return;
  }
  file_io.close();

  static auto song_validator = []() {
    nlohmann::json song_schema(
        {{"gain", get_number_schema("number", 0, MAX_GAIN)},
         {"starting_key",
          get_number_schema("number", MIN_STARTING_KEY, MAX_STARTING_KEY)},
         {"starting_tempo",
          get_number_schema("number", MIN_STARTING_TEMPO, MAX_STARTING_TEMPO)},
         {"starting_velocity", get_number_schema("number", 0, MAX_VELOCITY)}});
    add_row_array_schema<Chord>(song_schema);
    return make_validator(nlohmann::json({
                              "gain",
                              "starting_key",
                              "starting_tempo",
                              "starting_velocity",
                          }),
                          song_schema);
  }();
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    show_warning(file_menu, "Schema error", error.what());
    return;
  }

  controls.gain_editor.setValue(get_json_double(json_song, "gain"));
  controls.starting_key_editor.setValue(
      get_json_double(json_song, "starting_key"));
  controls.starting_velocity_editor.setValue(
      get_json_double(json_song, "starting_velocity"));
  controls.starting_tempo_editor.setValue(
      get_json_double(json_song, "starting_tempo"));

  if (number_of_chords > 0) {
    chords_model.remove_rows(0, number_of_chords);
  }

  if (json_song.contains("chords")) {
    chords_model.insert_json_rows(0, json_song["chords"]);
  }

  file_menu.current_file = filename;

  undo_stack.clear();
  undo_stack.setClean();
}

struct SongEditor : public QMainWindow {
  Q_OBJECT;

public:
  // data
  Song song;
  Player player = Player(*this);

  // folder/file fields

  // const fields
  QUndoStack &undo_stack = *(new QUndoStack(this));

  FileMenu &file_menu = *(new FileMenu("&File", *this));
  EditMenu &edit_menu = *(new EditMenu("&Edit", *this, undo_stack));
  PlayMenu &play_menu = *(new PlayMenu("&Play", *this));

  Controls &controls = *(new Controls);

  SwitchTable switch_table = SwitchTable(*this, undo_stack, song);

  explicit SongEditor() {
    auto &paste_menu = edit_menu.paste_menu;
    auto &insert_menu = edit_menu.insert_menu;

    auto &chords_model = switch_table.chords_model;

    auto &gain_editor = controls.gain_editor;
    auto &starting_key_editor = controls.starting_key_editor;
    auto &starting_velocity_editor = controls.starting_velocity_editor;
    auto &starting_tempo_editor = controls.starting_tempo_editor;

    get_reference(statusBar()).showMessage("");

    auto &menu_bar = get_reference(menuBar());

    menu_bar.addMenu(&file_menu);
    menu_bar.addMenu(&edit_menu);
    menu_bar.addMenu(&play_menu);

    auto &dock_widget = *(new QDockWidget(QObject::tr("Controls")));
    dock_widget.setWidget(&controls);
    dock_widget.setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, &dock_widget);

    setWindowTitle("Justly");
    setCentralWidget(&switch_table);
    resize(get_reference(QGuiApplication::primaryScreen())
               .availableGeometry()
               .size());

    QObject::connect(
        &file_menu.open_action, &QAction::triggered, this, [this]() {
          if (can_discard_changes(*this, undo_stack)) {
            auto &dialog = make_file_dialog(
                *this, "Open — Justly", file_menu.current_folder,
                "JSON file (*.json)", QFileDialog::AcceptOpen, ".json",
                QFileDialog::ExistingFile);
            if (dialog.exec() != 0) {
              open_file_split(undo_stack, file_menu, controls, switch_table,
                              get_selected_file(file_menu, dialog));
            }
          }
        });

    QObject::connect(&file_menu.save_action, &QAction::triggered, this,
                     [this]() {
                       save_song_as_file(undo_stack, file_menu, song, player,
                                         file_menu.current_file);
                     });

    QObject::connect(
        &file_menu.save_as_action, &QAction::triggered, this, [this]() {
          auto &dialog = make_file_dialog(
              *this, "Save As — Justly", "JSON file (*.json)",
              file_menu.current_folder, QFileDialog::AcceptSave, ".json",
              QFileDialog::AnyFile);

          if (dialog.exec() != 0) {
            save_song_as_file(undo_stack, file_menu, song, player,
                              get_selected_file(file_menu, dialog));
          }
        });

    QObject::connect(
        &file_menu.export_action, &QAction::triggered, this, [this]() {
          auto &dialog = make_file_dialog(
              *this, "Export — Justly", "WAV file (*.wav)",
              file_menu.current_folder, QFileDialog::AcceptSave, ".wav",
              QFileDialog::AnyFile);
          dialog.setLabelText(QFileDialog::Accept, "Export");
          if (dialog.exec() != 0) {
            export_song_to_file(player, song,
                                get_selected_file(file_menu, dialog));
          }
        });

    QObject::connect(&edit_menu.cut_action, &QAction::triggered, this,
                     [this]() {
                       copy_selection(switch_table);
                       delete_cells(undo_stack, switch_table);
                     });

    QObject::connect(&edit_menu.copy_action, &QAction::triggered, this,
                     [this]() { copy_selection(switch_table); });

    QObject::connect(
        &paste_menu.paste_over_action, &QAction::triggered, this, [this]() {
          auto first_row_number = get_only_range(switch_table).top();
          QUndoCommand *undo_command = nullptr;
          switch (switch_table.current_model_type) {
          case chords_type:
            undo_command = paste_cells_template(*this, first_row_number,
                                                switch_table.chords_model);
            break;
          case pitched_notes_type:
            undo_command = paste_cells_template(
                *this, first_row_number, switch_table.pitched_notes_model);
            break;
          case unpitched_notes_type:
            undo_command = paste_cells_template(
                *this, first_row_number, switch_table.unpitched_notes_model);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          if (undo_command == nullptr) {
            return;
          }
          undo_stack.push(undo_command);
        });

    QObject::connect(&paste_menu.paste_into_action, &QAction::triggered, this,
                     [this]() { paste_insert(undo_stack, switch_table, 0); });

    QObject::connect(
        &paste_menu.paste_after_action, &QAction::triggered, this, [this]() {
          paste_insert(undo_stack, switch_table, get_next_row(switch_table));
        });

    QObject::connect(&insert_menu.insert_after_action, &QAction::triggered,
                     this, [this]() {
                       insert_model_row(undo_stack, switch_table, song,
                                        get_next_row(switch_table));
                     });

    QObject::connect(
        &insert_menu.insert_into_action, &QAction::triggered, this,
        [this]() { insert_model_row(undo_stack, switch_table, song, 0); });

    QObject::connect(&edit_menu.delete_cells_action, &QAction::triggered, this,
                     [this]() { delete_cells(undo_stack, switch_table); });

    QObject::connect(
        &edit_menu.remove_rows_action, &QAction::triggered, this, [this]() {
          const auto &range = get_only_range(switch_table);
          auto first_row_number = range.top();
          auto number_of_rows = get_number_of_rows(range);

          QUndoCommand *undo_command = nullptr;
          switch (switch_table.current_model_type) {
          case chords_type:
            undo_command = remove_rows_template(
                switch_table.chords_model, first_row_number, number_of_rows);
            break;
          case pitched_notes_type:
            undo_command =
                remove_rows_template(switch_table.pitched_notes_model,
                                     first_row_number, number_of_rows);
            break;
          case unpitched_notes_type:
            undo_command =
                remove_rows_template(switch_table.unpitched_notes_model,
                                     first_row_number, number_of_rows);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          undo_stack.push(undo_command);
        });

    QObject::connect(
        &edit_menu.back_to_chords_action, &QAction::triggered, this, [this]() {
          add_edit_children_or_back(
              undo_stack, switch_table, file_menu, edit_menu, play_menu,
              switch_table.current_chord_number,
              switch_table.current_model_type == pitched_notes_type, true);
        });

    QObject::connect(
        &play_menu.play_action, &QAction::triggered, this, [this]() {
          const auto current_model_type = switch_table.current_model_type;

          const auto &range = get_only_range(switch_table);
          auto first_row_number = range.top();
          auto number_of_rows = get_number_of_rows(range);

          stop_playing(player.event_pointer, player.sequencer_pointer);
          initialize_play(player, song);

          if (current_model_type == chords_type) {
            modulate_before_chord(player, song, first_row_number);
            play_chords(player, song, first_row_number, number_of_rows);
          } else {
            auto current_chord_number = switch_table.current_chord_number;
            modulate_before_chord(player, song, current_chord_number);
            const auto &chord = song.chords.at(current_chord_number);
            modulate(player, chord);
            if (current_model_type == pitched_notes_type) {
              play_notes(player, current_chord_number, chord.pitched_notes,
                         first_row_number, number_of_rows);
            } else {
              play_notes(player, current_chord_number, chord.unpitched_notes,
                         first_row_number, number_of_rows);
            }
          }
        });

    QObject::connect(
        &play_menu.stop_playing_action, &QAction::triggered, this, [this]() {
          stop_playing(player.event_pointer, player.sequencer_pointer);
        });

    QObject::connect(&gain_editor, &QDoubleSpinBox::valueChanged, this,
                     [this](double new_value) {
                       set_double(undo_stack, song, player,
                                  controls.gain_editor, gain_id,
                                  player_get_gain(player), new_value);
                     });
    QObject::connect(&starting_key_editor, &QDoubleSpinBox::valueChanged, this,
                     [this](double new_value) {
                       set_double(undo_stack, song, player,
                                  controls.starting_key_editor, starting_key_id,
                                  song.starting_key, new_value);
                     });
    QObject::connect(&starting_velocity_editor, &QDoubleSpinBox::valueChanged,
                     this, [this](double new_value) {
                       set_double(undo_stack, song, player,
                                  controls.starting_velocity_editor,
                                  starting_velocity_id, song.starting_velocity,
                                  new_value);
                     });

    QObject::connect(
        &starting_tempo_editor, &QDoubleSpinBox::valueChanged, this,
        [this](double new_value) {
          set_double(undo_stack, song, player, controls.starting_tempo_editor,
                     starting_tempo_id, song.starting_tempo, new_value);
        });

    gain_editor.setValue(DEFAULT_GAIN);
    starting_key_editor.setValue(song.starting_key);
    starting_velocity_editor.setValue(song.starting_velocity);
    starting_tempo_editor.setValue(song.starting_tempo);

    set_model(switch_table, edit_menu, play_menu, chords_model);

    connect_model(switch_table, edit_menu, play_menu, chords_model);
    connect_model(switch_table, edit_menu, play_menu,
                  switch_table.pitched_notes_model);
    connect_model(switch_table, edit_menu, play_menu,
                  switch_table.unpitched_notes_model);

    QObject::connect(
        &switch_table.table_view, &QAbstractItemView::doubleClicked, this,
        [this](const QModelIndex &index) {
          if (switch_table.current_model_type == chords_type) {
            auto column = index.column();
            auto is_pitched = column == chord_pitched_notes_column;
            if (is_pitched || (column == chord_unpitched_notes_column)) {
              add_edit_children_or_back(undo_stack, switch_table, file_menu,
                                        edit_menu, play_menu, index.row(),
                                        is_pitched, false);
            }
          }
        });

    QObject::connect(&undo_stack, &QUndoStack::cleanChanged, this, [this]() {
      file_menu.save_action.setEnabled(!undo_stack.isClean() &&
                                       !file_menu.current_file.isEmpty());
    });

    undo_stack.clear();
    undo_stack.setClean();
  };

  ~SongEditor() override { undo_stack.disconnect(); }

  // prevent moving and copying
  SongEditor(const SongEditor &) = delete;
  auto operator=(const SongEditor &) -> SongEditor = delete;
  SongEditor(SongEditor &&) = delete;
  auto operator=(SongEditor &&) -> SongEditor = delete;

  void closeEvent(QCloseEvent *close_event_pointer) override {
    if (!can_discard_changes(*this, undo_stack)) {
      get_reference(close_event_pointer).ignore();
      return;
    }
    QMainWindow::closeEvent(close_event_pointer);
  };
};

template <std::derived_from<Named> SubNamed>
struct NamedEditor : public QComboBox {
  explicit NamedEditor(QWidget *parent_pointer) : QComboBox(parent_pointer) {
    static auto names_model = []() {
      const auto &all_nameds = SubNamed::get_all_nameds();
      QList<QString> names({""});
      std::transform(all_nameds.cbegin(), all_nameds.cend(),
                     std::back_inserter(names), [](const SubNamed &item) {
                       return QObject::tr(item.name.toStdString().c_str());
                     });
      return QStringListModel(names);
    }();
    setModel(&names_model);
    prevent_compression(*this);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  }

  [[nodiscard]] auto value() const -> const SubNamed * {
    auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &SubNamed::get_all_nameds().at(row - 1);
  }

  void setValue(const SubNamed *new_value) {
    setCurrentIndex(new_value == nullptr
                        ? 0
                        : static_cast<int>(std::distance(
                              SubNamed::get_all_nameds().data(), new_value)) +
                              1);
  }
};

struct InstrumentEditor : public NamedEditor<Instrument> {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *parent_pointer)
      : NamedEditor<Instrument>(parent_pointer) {}
};

struct PercussionSetEditor : public NamedEditor<PercussionSet> {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *parent_pointer_input)
      : NamedEditor<PercussionSet>(parent_pointer_input) {}
};

struct PercussionInstrumentEditor : public NamedEditor<PercussionInstrument> {
  Q_OBJECT
  Q_PROPERTY(
      const PercussionInstrument *value READ value WRITE setValue USER true)
public:
  explicit PercussionInstrumentEditor(QWidget *parent_pointer)
      : NamedEditor<PercussionInstrument>(parent_pointer) {}
};

struct AbstractRationalEditor : public QFrame {
  QSpinBox &numerator_box = *(new QSpinBox);
  QSpinBox &denominator_box = *(new QSpinBox);
  QHBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit AbstractRationalEditor(QWidget *parent_pointer)
      : QFrame(parent_pointer) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    numerator_box.setMinimum(1);
    numerator_box.setMaximum(MAX_RATIONAL_NUMERATOR);

    denominator_box.setMinimum(1);
    denominator_box.setMaximum(MAX_RATIONAL_DENOMINATOR);

    row_layout.addWidget(&numerator_box);
    row_layout.addWidget(
        new QLabel("/")); // NOLINT(cppcoreguidelines-owning-memory)
    row_layout.addWidget(&denominator_box);
  }

  void setValue(const AbstractRational &new_value) const {
    numerator_box.setValue(new_value.numerator);
    denominator_box.setValue(new_value.denominator);
  }
};

struct IntervalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

public:
  QSpinBox &octave_box = *(new QSpinBox);

  explicit IntervalEditor(QWidget *parent_pointer)
      : AbstractRationalEditor(parent_pointer) {
    octave_box.setMinimum(-MAX_OCTAVE);
    octave_box.setMaximum(MAX_OCTAVE);

    row_layout.addWidget(
        new QLabel("o")); // NOLINT(cppcoreguidelines-owning-memory)
    row_layout.addWidget(&octave_box);

    prevent_compression(*this);
  }

  [[nodiscard]] auto value() const -> Interval {
    return {numerator_box.value(), denominator_box.value(), octave_box.value()};
  }

  void setValue(const Interval &new_value) const {
    AbstractRationalEditor::setValue(new_value);
    octave_box.setValue(new_value.octave);
  }
};

struct RationalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  explicit RationalEditor(QWidget *parent_pointer)
      : AbstractRationalEditor(parent_pointer) {
    prevent_compression(*this);
  }

  [[nodiscard]] auto value() const -> Rational {
    return {numerator_box.value(), denominator_box.value()};
  }
};

void set_up() {
  QApplication::setApplicationDisplayName("Justly");

  auto icon_file = QDir(QCoreApplication::applicationDirPath())
                       .filePath("../share/Justly.svg");
  Q_ASSERT(QFile::exists(icon_file));
  QIcon icon(icon_file);
  Q_ASSERT(!icon.isNull());
  QApplication::setWindowIcon(icon);

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
  QMetaType::registerConverter<const Instrument *, QString>(&get_name_or_empty);
  QMetaType::registerConverter<const PercussionInstrument *, QString>(
      &get_name_or_empty);
  QMetaType::registerConverter<const PercussionSet *, QString>(
      &get_name_or_empty);

  auto &factory = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QItemEditorFactory);
  factory.registerEditor(
      qMetaTypeId<Rational>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          RationalEditor>);
  factory.registerEditor(
      qMetaTypeId<const PercussionInstrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionInstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<const PercussionSet *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionSetEditor>);
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
  factory.registerEditor(
      qMetaTypeId<int>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QSpinBox>);
  QItemEditorFactory::setDefaultFactory(&factory);
}

auto make_song_editor() -> SongEditor & {
  return *(new SongEditor); // NOLINT(cppcoreguidelines-owning-memory)
}

void show_song_editor(SongEditor &song_editor) { return song_editor.show(); }

void delete_song_editor(SongEditor &song_editor) {
  delete &song_editor; // NOLINT(cppcoreguidelines-owning-memory)
}

auto get_table_view(const SongEditor &song_editor) -> QAbstractItemView & {
  return song_editor.switch_table.table_view;
}

auto get_chords_model(SongEditor &song_editor) -> QAbstractItemModel & {
  return song_editor.switch_table.chords_model;
}

auto get_pitched_notes_model(SongEditor &song_editor) -> QAbstractItemModel & {
  return song_editor.switch_table.pitched_notes_model;
}

auto get_unpitched_notes_model(SongEditor &song_editor)
    -> QAbstractItemModel & {
  return song_editor.switch_table.unpitched_notes_model;
}

void trigger_edit_pitched_notes(SongEditor &song_editor, int chord_number) {
  double_click_chord_column(song_editor.switch_table, chord_number,
                            chord_pitched_notes_column);
}

void trigger_edit_unpitched_notes(SongEditor &song_editor, int chord_number) {
  double_click_chord_column(song_editor.switch_table, chord_number,
                            chord_unpitched_notes_column);
}

void trigger_back_to_chords(SongEditor &song_editor) {
  song_editor.edit_menu.back_to_chords_action.trigger();
}

auto get_gain(const SongEditor &song_editor) -> double {
  return player_get_gain(song_editor.player);
}

auto get_starting_key(const SongEditor &song_editor) -> double {
  return song_editor.song.starting_key;
}

auto get_starting_velocity(const SongEditor &song_editor) -> double {
  return song_editor.song.starting_velocity;
}

auto get_starting_tempo(const SongEditor &song_editor) -> double {
  return song_editor.song.starting_tempo;
}

auto get_current_file(const SongEditor &song_editor) -> QString {
  return song_editor.file_menu.current_file;
}

void set_gain(const SongEditor &song_editor, double new_value) {
  song_editor.controls.gain_editor.setValue(new_value);
}

void set_starting_key(const SongEditor &song_editor, double new_value) {
  song_editor.controls.starting_key_editor.setValue(new_value);
}

void set_starting_velocity(const SongEditor &song_editor, double new_value) {
  song_editor.controls.starting_velocity_editor.setValue(new_value);
}

void set_starting_tempo(const SongEditor &song_editor, double new_value) {
  song_editor.controls.starting_tempo_editor.setValue(new_value);
}

auto create_editor(const QAbstractItemView &table_view,
                   QModelIndex index) -> QWidget & {
  auto &delegate = get_reference(table_view.itemDelegate());
  auto &cell_editor = get_reference(delegate.createEditor(
      &get_reference(table_view.viewport()), QStyleOptionViewItem(), index));
  delegate.setEditorData(&cell_editor, index);
  return cell_editor;
}

void set_editor(const QAbstractItemView &table_view, QWidget &cell_editor,
                QModelIndex index, const QVariant &new_value) {
  cell_editor.setProperty(
      get_const_reference(cell_editor.metaObject()).userProperty().name(),
      new_value);
  get_reference(table_view.itemDelegate())
      .setModelData(&cell_editor, table_view.model(), index);
}

void undo(const SongEditor &song_editor) { song_editor.undo_stack.undo(); }

void trigger_insert_after(SongEditor &song_editor) {
  song_editor.edit_menu.insert_menu.insert_after_action.trigger();
}

void trigger_insert_into(SongEditor &song_editor) {
  song_editor.edit_menu.insert_menu.insert_into_action.trigger();
}

void trigger_delete_cells(SongEditor &song_editor) {
  song_editor.edit_menu.delete_cells_action.trigger();
}

void trigger_remove_rows(SongEditor &song_editor) {
  song_editor.edit_menu.remove_rows_action.trigger();
}

void trigger_cut(SongEditor &song_editor) {
  song_editor.edit_menu.cut_action.trigger();
}

void trigger_copy(SongEditor &song_editor) {
  song_editor.edit_menu.copy_action.trigger();
}

void trigger_paste_over(SongEditor &song_editor) {
  song_editor.edit_menu.paste_menu.paste_over_action.trigger();
}

void trigger_paste_into(SongEditor &song_editor) {
  song_editor.edit_menu.paste_menu.paste_into_action.trigger();
}

void trigger_paste_after(SongEditor &song_editor) {
  song_editor.edit_menu.paste_menu.paste_after_action.trigger();
}

void trigger_save(SongEditor &song_editor) {
  song_editor.file_menu.save_action.trigger();
}

void trigger_play(SongEditor &song_editor) {
  song_editor.play_menu.play_action.trigger();
}

void trigger_stop_playing(SongEditor &song_editor) {
  song_editor.play_menu.stop_playing_action.trigger();
}

void open_file(SongEditor &song_editor, const QString &filename) {
  open_file_split(song_editor.undo_stack, song_editor.file_menu,
                  song_editor.controls, song_editor.switch_table, filename);
}

void save_as_file(SongEditor &song_editor, const QString &filename) {
  save_song_as_file(song_editor.undo_stack, song_editor.file_menu,
                    song_editor.song, song_editor.player, filename);
}

void export_to_file(SongEditor &song_editor, const QString &output_file) {
  export_song_to_file(song_editor.player, song_editor.song, output_file);
}

#include "justly.moc"