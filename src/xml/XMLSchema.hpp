#pragma once

#include "xml/XMLParserContext.hpp"

class XMLSchema {
public:
  _xmlSchema * const internal_pointer;

  explicit XMLSchema(const XMLParserContext& context)
      : internal_pointer(xmlSchemaParse(context.internal_pointer)) {}

  ~XMLSchema() {
    xmlSchemaFree(internal_pointer);
  }

  NO_MOVE_COPY(XMLSchema)
};
