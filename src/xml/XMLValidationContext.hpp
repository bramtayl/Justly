#pragma once

#include "xml/XMLSchema.hpp"

class XMLValidationContext {
public:
  _xmlSchemaValidCtxt * const internal_pointer;

  explicit XMLValidationContext(XMLSchema& schema)
      : internal_pointer(xmlSchemaNewValidCtxt(schema.internal_pointer)) {}

  ~XMLValidationContext() { xmlSchemaFreeValidCtxt(internal_pointer); }

  NO_MOVE_COPY(XMLValidationContext)
};
