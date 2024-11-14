#pragma once

#include <QByteArray>
#include <QMetaType>

#include "named/program/Program.hpp"

template <typename T> class QList;

struct PercussionSet : public Program {
  PercussionSet(const char* name, short bank_number, short preset_number);
  [[nodiscard]] static auto get_all_nameds() -> const QList<PercussionSet> &;
  [[nodiscard]] static auto get_field_name() -> const char*;
};

Q_DECLARE_METATYPE(const PercussionSet *);
