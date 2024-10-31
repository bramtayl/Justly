
#pragma once

#include <QObject>

#include "named/NamedEditor.hpp"
#include "percussion_set/PercussionSet.hpp" // IWYU pragma: keep

class QWidget;

struct PercussionSetEditor : public NamedEditor<PercussionSet> {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *parent_pointer_input = nullptr);
};
