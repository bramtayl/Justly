#pragma once
#include "qcolor.h"
#include <QtCore/QtGlobal>

#if defined(JUSTLY_LIBRARY)
#define JUSTLY_EXPORT Q_DECL_EXPORT
#else
#define JUSTLY_EXPORT Q_DECL_IMPORT
#endif

#define NO_MOVE_COPY(class_name)                          \
  class_name(const class_name&) = delete;                 \
  auto operator=(const class_name&)->class_name = delete; \
  class_name(class_name&&) = delete;                      \
  auto operator=(class_name&&)->class_name = delete;

const auto NON_DEFAULT_COLOR = QColor(Qt::black);
const auto DEFAULT_COLOR = QColor(Qt::lightGray);

const auto NOTE_CHORD_COLUMNS = 7;

const auto MAX_NUMERATOR = 199;

const auto MAX_DENOMINATOR = 199;

const auto MIN_OCTAVE = -9;
const auto MAX_OCTAVE = 9;