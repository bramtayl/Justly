#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <cstdint>

template <typename T> class QList;

struct PercussionSet {
  QString name;
  int16_t bank_number = -1;
  int16_t preset_number = -1;
  bool is_percussion = false;
};

[[nodiscard]] auto
get_all_percussion_sets() -> const QList<PercussionSet> &;

[[nodiscard]] auto get_percussion_set_pointer(const QString &name)
    -> const PercussionSet *;

Q_DECLARE_METATYPE(const PercussionSet *);
