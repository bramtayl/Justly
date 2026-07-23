#pragma once

#include <libxml/xmlschemas.h>

#include "other/helpers.hpp"
#include "xml/XMLParserContext.hpp"

struct _xmlSchema;

class XMLSchema {
public:
  _xmlSchema *const internal_pointer;

  explicit XMLSchema(const XMLParserContext &context)
      : internal_pointer(xmlSchemaParse(context.internal_pointer)) {}

  ~XMLSchema() { xmlSchemaFree(internal_pointer); }

  NO_MOVE_COPY(XMLSchema)
};
