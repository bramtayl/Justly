#include "editors/sizes.hpp"

#include <qlineedit.h>  // for QLineEdit

#include <memory>  // for make_unique, unique_ptr

#include "editors/InstrumentEditor.hpp"  // for InstrumentEditor
#include "editors/IntervalEditor.hpp"    // for IntervalEditor
#include "editors/RationalEditor.hpp"    // for RationalEditor

auto get_interval_size() -> QSize {
  return std::make_unique<IntervalEditor>()->sizeHint();
};
auto get_rational_size() -> QSize {
  return std::make_unique<RationalEditor>()->sizeHint();
};
auto get_instrument_size() -> QSize {
  return std::make_unique<InstrumentEditor>()->sizeHint();
};
auto get_words_size() -> QSize {
  return std::make_unique<QLineEdit>()->sizeHint();
};