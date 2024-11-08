#pragma once

#include <QByteArray>
#include <QMetaType>

#include "named/program/Program.hpp"

template <typename T> class QList;

struct Instrument : public Program {
  Instrument(const char* name, short bank_number, short preset_number);
  [[nodiscard]] static auto get_all_nameds() -> const QList<Instrument> &;

  [[nodiscard]] static auto get_field_name() -> const char*;
  [[nodiscard]] static auto get_type_name() -> const char*;
  [[nodiscard]] static auto get_missing_error() -> const char*;
  [[nodiscard]] static auto get_default() -> const char*;
};

Q_DECLARE_METATYPE(const Instrument *);
