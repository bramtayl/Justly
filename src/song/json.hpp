#include <iterator>                                 // for back_inserter
#include <memory>                                   // for unique_ptr
#include <nlohmann/detail/iterators/iter_impl.hpp>  // for iter_impl
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>
#include <vector>

#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"  // for NoteChord

template <typename ObjectType>
[[nodiscard]] auto to_json(
    const std::vector<std::unique_ptr<ObjectType>> &object_pointers,
    size_t first_object_number, size_t number_of_objects) -> nlohmann::json {
  nlohmann::json json_objects;
  auto end_number = first_object_number + number_of_objects;
  Q_ASSERT(0 <= first_object_number);
  Q_ASSERT(first_object_number < object_pointers.size());

  Q_ASSERT(0 < end_number);
  Q_ASSERT(end_number <= object_pointers.size());

  std::transform(
      object_pointers.cbegin() + first_object_number,
      object_pointers.cbegin() + first_object_number + number_of_objects,
      std::back_inserter(json_objects),
      [](const std::unique_ptr<ObjectType> &object_pointer) {
        return object_pointer->json();
      });
  return json_objects;
}

template <typename ObjectType>
void from_json(
    std::vector<std::unique_ptr<ObjectType>> *object_pointers_pointer,
    int first_object_number, const nlohmann::json &json_objects) {
  Q_ASSERT(object_pointers_pointer != nullptr);

  Q_ASSERT(0 <= first_object_number);
  Q_ASSERT(static_cast<size_t>(first_object_number) <= object_pointers_pointer->size());
  std::transform(
      json_objects.cbegin(),
      json_objects.cbegin() + static_cast<int>(json_objects.size()),
      std::inserter(*object_pointers_pointer,
                    object_pointers_pointer->begin() + first_object_number),
      [](const nlohmann::json &json_object) {
        return std::make_unique<ObjectType>(json_object);
      });
}
