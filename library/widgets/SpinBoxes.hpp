#pragma once

#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QtAssert>
#include <QtGui/QUndoStack>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include <fluidsynth.h>
#include <fluidsynth/synth.h>

#include "actions/ChangeId.hpp"
#include "actions/SetDouble.hpp"
#include "other/Song.hpp"
#include "rows/Note.hpp"
#include "sound/FluidSynth.hpp"

static const auto DEFAULT_GAIN = 5;
static const auto GAIN_STEP = 0.1;
static const auto MAX_GAIN = 10;
static const auto MAX_KEY = 999;
static const auto MAX_TEMPO = 999;

void clear_and_clean(QUndoStack &undo_stack);

struct SpinBoxes : public QWidget {
  QDoubleSpinBox &gain_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_key_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_velocity_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_tempo_editor = *(new QDoubleSpinBox);
  QFormLayout &spin_boxes_form = *(new QFormLayout(this));

  explicit SpinBoxes(Song &song, FluidSynth &synth, QUndoStack &undo_stack);
};
