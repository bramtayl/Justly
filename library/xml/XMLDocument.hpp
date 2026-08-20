#pragma once

#include "other/helpers.hpp"

class XMLDocument {
 public:
  xmlDoc* internal_pointer;

  XMLDocument() : internal_pointer(xmlNewDoc(nullptr)) {}

  explicit XMLDocument(xmlDoc* internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLDocument();

  NO_MOVE_COPY(XMLDocument)
};

[[nodiscard]] auto get_root(const XMLDocument& document) -> xmlNode&;

[[nodiscard]] auto make_root(XMLDocument& document, const char* field_name)
    -> xmlNode&;

[[nodiscard]] auto document_to_byte_array(const XMLDocument& document)
    -> QByteArray;

// rejects buffers that don't fit in the int size xmlReadMemory takes --
// casting an oversized qsizetype down to int would both under-report the
// buffer length and cause xmlReadMemory to read past the end of it
[[nodiscard]] auto xml_bytes_size_is_safe(qsizetype size) -> bool;

[[nodiscard]] auto read_xml_document(const QByteArray& bytes) -> XMLDocument;

// looks for a direct voice_number child of note_node and renumbers it (or,
// for a removal that swallows it, zeroes it) the same way
// InsertVoiceRow/RemoveVoiceRows renumber notes still in song.chords; used
// both for a flat copied note row and for a single pitched_note/unpitched_note
// nested inside a copied chord. reassigned_count is bumped whenever a
// voice_number collapses to 0, so the caller can warn about it the same way
// RemoveVoiceRows warns about a live note being reassigned
void find_and_process_voice_number(xmlNode& note_node, int first_row_number,
                                   int last_removed_row, int number_of_rows,
                                   bool is_insertion, bool& changed,
                                   int& reassigned_count);
