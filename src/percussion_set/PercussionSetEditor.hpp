
#pragma once

#include <QComboBox>
#include <QObject>

#include "other/ComboboxEditor.hpp"
#include "percussion_set/PercussionSet.hpp"

class QWidget;

struct PercussionSetEditor : public ComboboxEditor<PercussionSet> {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *parent_pointer_input = nullptr)
      : ComboboxEditor<PercussionSet>(get_all_percussion_sets(),
                                      parent_pointer_input){};
};
