#include <QtGlobal>
#include <cstddef>

auto to_size_t(int input) -> size_t {
  Q_ASSERT(input >= 0);
  return static_cast<size_t>(input);
}
