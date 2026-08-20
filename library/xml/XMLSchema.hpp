#pragma once

#include "xml/XMLParserContext.hpp"

struct _xmlSchema;

class XMLSchema {
 public:
  _xmlSchema* const internal_pointer;

  explicit XMLSchema(const XMLParserContext& context)
      : internal_pointer(xmlSchemaParse(context.internal_pointer)) {}

  ~XMLSchema();

  NO_MOVE_COPY(XMLSchema)
};
