#pragma once

#include <QtCore/QByteArray>
#include <QtCore/qtypes.h>
#include <libxml/parser.h>
#include <limits>

#include "other/helpers.hpp"

class XMLDocument {
public:
  xmlDoc *internal_pointer;

  XMLDocument() : internal_pointer(xmlNewDoc(nullptr)) {}

  explicit XMLDocument(xmlDoc *internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLDocument();

  NO_MOVE_COPY(XMLDocument)
};

[[nodiscard]] auto get_root(const XMLDocument &document) -> xmlNode &;

[[nodiscard]] auto make_root(XMLDocument &document,
                             const char *field_name) -> xmlNode &;

[[nodiscard]] auto
document_to_byte_array(const XMLDocument &document) -> QByteArray;

// rejects buffers that don't fit in the int size xmlReadMemory takes --
// casting an oversized qsizetype down to int would both under-report the
// buffer length and cause xmlReadMemory to read past the end of it
[[nodiscard]] auto xml_bytes_size_is_safe(qsizetype size) -> bool;

[[nodiscard]] auto
read_xml_document(const QByteArray &bytes) -> XMLDocument;
