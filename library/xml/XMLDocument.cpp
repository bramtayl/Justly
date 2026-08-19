#include "xml/XMLDocument.hpp"

#include <libxml/parser.h>

#include <QtCore/QtTypes>
#include <limits>

#include "other/helpers.hpp"

XMLDocument::~XMLDocument() { xmlFreeDoc(internal_pointer); }

auto get_root(const XMLDocument& document) -> xmlNode& {
  return get_reference(xmlDocGetRootElement(document.internal_pointer));
}

auto make_root(XMLDocument& document, const char* field_name) -> xmlNode& {
  xmlDocSetRootElement(document.internal_pointer,
                       xmlNewNode(nullptr, c_string_to_xml_string(field_name)));
  return get_root(document);
}

auto document_to_byte_array(const XMLDocument& document) -> QByteArray {
  XMLString char_buffer;
  auto buffer_size = 0;
  xmlDocDumpMemory(document.internal_pointer, &char_buffer.internal_pointer,
                   &buffer_size);
  return {
      reinterpret_cast<
          const char*>(  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
          char_buffer.internal_pointer),
      buffer_size};
}

auto xml_bytes_size_is_safe(qsizetype size) -> bool {
  return size <= static_cast<qsizetype>(std::numeric_limits<int>::max());
}

auto read_xml_document(const QByteArray& bytes) -> XMLDocument {
  if (!xml_bytes_size_is_safe(bytes.size())) {
    return XMLDocument(nullptr);
  }
  return XMLDocument(xmlReadMemory(
      bytes.constData(), static_cast<int>(bytes.size()), nullptr, nullptr, 0));
}
