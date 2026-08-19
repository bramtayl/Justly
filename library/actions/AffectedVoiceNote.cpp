#include "actions/AffectedVoiceNote.hpp"

#include <libxml/parser.h>
#include <string>

#include "other/helpers.hpp"

void find_and_process_voice_number(xmlNode &note_node, const int first_row_number,
                                   const int last_removed_row,
                                   const int number_of_rows, const bool is_insertion,
                                   bool &changed, int &reassigned_count) {
  auto *note_field_pointer = xmlFirstElementChild(&note_node);
  while (note_field_pointer != nullptr) {
    auto &note_field_node = get_reference(note_field_pointer);
    if (get_xml_name(note_field_node) == "voice_number") {
      const auto voice_number = xml_to_int(note_field_node);
      auto new_voice_number = voice_number;
      if (is_insertion) {
        if (voice_number >= first_row_number) {
          new_voice_number = voice_number + number_of_rows;
        }
      } else if (voice_number > last_removed_row) {
        new_voice_number = voice_number - number_of_rows;
      } else if (voice_number >= first_row_number) {
        new_voice_number = 0;
        reassigned_count = reassigned_count + 1;
      }
      xmlNodeSetContent(
          &note_field_node,
          c_string_to_xml_string(std::to_string(new_voice_number).c_str()));
      changed = true;
    }
    note_field_pointer = xmlNextElementSibling(note_field_pointer);
  }
}
