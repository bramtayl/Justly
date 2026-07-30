#pragma once

#include <QtCore/QByteArray>
#include <libxml/parser.h>

#include "other/helpers.hpp"

class XMLDocument {
public:
  xmlDoc *internal_pointer;

  XMLDocument() : internal_pointer(xmlNewDoc(nullptr)) {}

  explicit XMLDocument(xmlDoc *internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  ~XMLDocument() { xmlFreeDoc(internal_pointer); }

  NO_MOVE_COPY(XMLDocument)
};

[[nodiscard]] static auto get_root(const XMLDocument &document) -> auto & {
  return get_reference(xmlDocGetRootElement(document.internal_pointer));
}

[[nodiscard]] static inline auto make_root(XMLDocument &document,
                                           const char *field_name) -> auto & {
  xmlDocSetRootElement(document.internal_pointer,
                       xmlNewNode(nullptr, c_string_to_xml_string(field_name)));
  return get_root(document);
}

[[nodiscard]] static inline auto
document_to_byte_array(const XMLDocument &document) -> QByteArray {
  XMLString char_buffer;
  auto buffer_size = 0;
  xmlDocDumpMemory(document.internal_pointer, &char_buffer.internal_pointer,
                   &buffer_size);
  return {reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
              char_buffer.internal_pointer),
          buffer_size};
}

[[nodiscard]] static inline auto
read_xml_document(const QByteArray &bytes) -> XMLDocument {
  return XMLDocument(xmlReadMemory(bytes.constData(),
                                   static_cast<int>(bytes.size()), nullptr,
                                   nullptr, 0));
}
