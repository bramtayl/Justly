#pragma once

#include "xml/XMLSchema.hpp"

struct XMLValidationContext
    : CHandle<_xmlSchemaValidCtxt, xmlSchemaFreeValidCtxt> {
  explicit XMLValidationContext(XMLSchema& schema)
      : CHandle(xmlSchemaNewValidCtxt(schema.internal_pointer)) {}
};
