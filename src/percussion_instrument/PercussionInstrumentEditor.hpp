
#pragma once

#include <QObject>

#include "named/NamedEditor.hpp"
#include "percussion_instrument/PercussionInstrument.hpp" // IWYU pragma: keep

class QWidget;

struct PercussionInstrumentEditor : public NamedEditor<PercussionInstrument> {
  Q_OBJECT
  Q_PROPERTY(
      const PercussionInstrument *value READ value WRITE setValue USER true)
public:
  explicit PercussionInstrumentEditor(QWidget *parent_pointer);
};
