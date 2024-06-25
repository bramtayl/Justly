#include <qlineedit.h>

#include "editors/sizes.hpp"
#include "InstrumentEditor.hpp"
#include "IntervalEditor.hpp"
#include "RationalEditor.hpp"

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