#pragma once

#include "other/helpers.hpp"

struct XMLDocument : CHandle<xmlDoc, xmlFreeDoc> {
  XMLDocument() : CHandle(xmlNewDoc(nullptr)) {}

  explicit XMLDocument(xmlDoc* internal_pointer_input)
      : CHandle(internal_pointer_input) {}
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
