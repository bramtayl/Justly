#pragma once

#include <QByteArray>
#include <QMetaType>

#include "named/Named.hpp"

template <typename T> class QList;

struct PercussionInstrument : public Named {
  short midi_number;
  PercussionInstrument(const char* name, short midi_number);
  [[nodiscard]] static auto
  get_all_nameds() -> const QList<PercussionInstrument> &;

  [[nodiscard]] static auto get_field_name() -> const char*;
  [[nodiscard]] static auto get_type_name() -> const char*;
  [[nodiscard]] static auto get_missing_error() -> const char*;
  [[nodiscard]] static auto get_default() -> const char*;
};

Q_DECLARE_METATYPE(const PercussionInstrument *);
