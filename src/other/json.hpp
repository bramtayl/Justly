#include <iterator>  // for back_inserter
#include <memory>    // for unique_ptr
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"  // for NoteChord

template <typename ObjectType>
[[nodiscard]] auto objects_to_json(const std::vector<ObjectType> &objects,
                                   size_t first_object_number,
                                   size_t number_of_objects) -> nlohmann::json {
  nlohmann::json json_objects;
  auto end_number = first_object_number + number_of_objects;

  auto objects_size = objects.size();
  Q_ASSERT(first_object_number < objects_size);

  Q_ASSERT(0 < end_number);
  Q_ASSERT(end_number <= objects_size);

  std::transform(objects.cbegin() + first_object_number,
                 objects.cbegin() + first_object_number + number_of_objects,
                 std::back_inserter(json_objects),
                 [](const ObjectType &object) { return object.json(); });
  return json_objects;
}

template <typename ObjectType>
void insert_from_json(std::vector<ObjectType> &objects,
                      size_t first_object_number,
                      const nlohmann::json &json_objects) {
  Q_ASSERT(first_object_number <= objects.size());
  std::transform(json_objects.cbegin(),
                 json_objects.cbegin() + static_cast<int>(json_objects.size()),
                 std::inserter(objects, objects.begin() + first_object_number),
                 [](const nlohmann::json &json_object) {
                   return ObjectType(json_object);
                 });
}
