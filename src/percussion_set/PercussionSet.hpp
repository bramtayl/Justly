#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QList> // IWYU pragma: keep
#include <QString>

#include <cstdint>

struct PercussionSet {
  QString name;
  int16_t bank_number = -1;
  int16_t preset_number = -1;
  bool is_percussion = false;
};

[[nodiscard]] auto
get_all_percussion_sets() -> const QList<PercussionSet> &;

Q_DECLARE_METATYPE(const PercussionSet *);
