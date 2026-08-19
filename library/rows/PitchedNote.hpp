#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <QtWidgets/QMessageBox>
#include <cmath>
#include <fluidsynth.h>
#include <fluidsynth/event.h>
#include <fluidsynth/seq.h>
#include <libxml/parser.h>
#include <optional>

#include "cell_types/Interval.hpp"
#include "cell_types/Program.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Row.hpp"
#include "rows/Voice.hpp"
#include "sound/FluidEvent.hpp"
#include "sound/FluidSequencer.hpp"
#include "sound/PlayState.hpp"
#include "sound/Player.hpp"

static const auto BEND_PER_HALFSTEP = 4096;
static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_FREQUENCY = 12911.41; // MIDI 127 plus half step
static const auto QUARTER_STEP = 0.5;
static const auto ZERO_BEND_HALFSTEPS = 2;

[[nodiscard]] auto frequency_to_midi_number(const double key) -> double;

[[nodiscard]] auto
midi_number_to_frequency(const double midi_number) -> double;

[[nodiscard]] auto to_int(const double value) -> int;

void send_event_at(FluidSequencer &sequencer, FluidEvent &event,
                   const double time);

struct PitchedNote : Note {
  Interval interval;

  void from_xml(xmlNode &node) override;

  [[nodiscard]] static auto get_clipboard_schema() -> const char *;

  [[nodiscard]] static auto get_xml_field_name() -> const char *;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char *;

  [[nodiscard]] static auto get_cells_mime() -> const char *;

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool;

  [[nodiscard]] static auto get_pitched() -> const char *;

  [[nodiscard]] static auto is_pitched() -> bool;

  [[nodiscard]] auto
  get_closest_midi(QWidget &parent, Player &player,
                   const QList<UnpitchedVoice> & /*unpitched_voices*/,
                   const int channel_number, const int chord_number,
                   const int note_number) const
      -> std::optional<short> override;

  [[nodiscard]] auto
  get_program(const QList<PitchedVoice> &pitched_voices,
              const QList<UnpitchedVoice> & /*unpitched_voices*/) const
      -> const Program & override;

  [[nodiscard]] auto
  get_voice_velocity_ratio(const QList<PitchedVoice> &pitched_voices,
                           const QList<UnpitchedVoice> & /*unpitched_voices*/)
      const -> const Rational & override;

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override;

  void set_data(const int column_number, const QVariant &new_value) override;

  void copy_column_from(const PitchedNote &template_row,
                        const int column_number);

  void column_to_xml(xmlNode &node, const int column_number) const override;

  void to_xml(xmlNode &node) const override;
};
