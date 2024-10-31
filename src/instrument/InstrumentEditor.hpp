
#pragma once

#include <QObject>

#include "instrument/Instrument.hpp" // IWYU pragma: keep
#include "named/NamedEditor.hpp"

class QWidget;

struct InstrumentEditor : public NamedEditor<Instrument> {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *parent_pointer_input = nullptr);
};
