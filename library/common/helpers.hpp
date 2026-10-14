#pragma once

#include <QtCore/QDir>
#include <utility>

class QClipboard;
class QAbstractItemModel;
class QItemSelectionRange;
class QUndoCommand;
class QUndoStack;

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)
#define NO_COPY(classname)              \
  classname(const classname&) = delete; \
  auto operator=(const classname&)->classname = delete;

#define NO_MOVE_COPY(classname)    \
  NO_COPY(classname)               \
  classname(classname&&) = delete; \
  auto operator=(classname&&)->classname = delete;
// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

// owns a pointer from a C library, freeing it with free_function, which is
// skipped for null since not every library's free function accepts null;
// moving leaves the source null
template <typename Pointee, auto free_function>
struct CHandle {
  Pointee* internal_pointer;

  explicit CHandle(Pointee* const internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  CHandle(CHandle&& other) noexcept
      : internal_pointer(std::exchange(other.internal_pointer, nullptr)) {}

  auto operator=(CHandle&& other) noexcept -> CHandle& {
    if (this != &other) {
      reset();
      internal_pointer = std::exchange(other.internal_pointer, nullptr);
    }
    return *this;
  }

  ~CHandle() { reset(); }

  void reset() {
    if (internal_pointer != nullptr) {
      free_function(internal_pointer);
    }
    internal_pointer = nullptr;
  }

  NO_COPY(CHandle)
};

template <typename Thing>
[[nodiscard]] auto get_reference(Thing* thing_pointer) -> auto& {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

[[nodiscard]] auto to_int(double value) -> int;

[[nodiscard]] auto get_clipboard() -> QClipboard&;

// forgets every undo command and marks the result as saved, e.g. after
// loading a song
void clear_and_clean(QUndoStack& undo_stack);

// pushes undo_command unless it's null, e.g. because making it was cancelled
void maybe_push(QUndoStack& undo_stack, QUndoCommand* undo_command);

[[nodiscard]] auto get_number_of_rows(const QItemSelectionRange& range) -> int;

[[nodiscard]] auto make_range(QAbstractItemModel& model, int first_row_number,
                              int number_of_rows, int left_column,
                              int right_column) -> QItemSelectionRange;

template <typename Item>
[[nodiscard]] auto copy_items(const QList<Item>& items,
                              const int first_row_number,
                              const int number_of_rows) {
  Q_ASSERT(first_row_number >= 0);
  Q_ASSERT(number_of_rows >= 0);
  Q_ASSERT(first_row_number + number_of_rows <= items.size());

  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

template <typename SubType>
[[nodiscard]] auto variant_to(const QVariant& variant) {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}

template <typename SubNamed>
concept NamedInterface = requires(SubNamed named) { named.name; };

template <NamedInterface Named>
auto get_named_index(const QList<Named>& nameds, const QString& name) -> auto {
  return std::find_if(
      nameds.cbegin(), nameds.cend(),
      [&name](const Named& named) -> auto { return named.name == name; });
}

template <NamedInterface Named>
[[nodiscard]] auto get_names(const QList<Named>& nameds) {
  QList<QString> names;
  std::transform(nameds.cbegin(), nameds.cend(), std::back_inserter(names),
                 [](const Named& named) -> auto { return named.name; });
  return names;
}

// installed layout is <prefix>/share next to the binary's folder, except
// inside a macOS app bundle, where resources live in Contents/Resources
// rather than alongside Contents/MacOS
[[nodiscard]] auto get_share_folder() -> QDir;

[[nodiscard]] auto get_share_file(const char* file_name) -> std::string;
