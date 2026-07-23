#pragma once

#include <libxml/xmlschemas.h>

#include "other/helpers.hpp"
#include "xml/XMLSchema.hpp"

struct _xmlSchemaValidCtxt;

class XMLValidationContext {
public:
  _xmlSchemaValidCtxt *const internal_pointer;

  explicit XMLValidationContext(XMLSchema &schema)
      : internal_pointer(xmlSchemaNewValidCtxt(schema.internal_pointer)) {}

  ~XMLValidationContext() { xmlSchemaFreeValidCtxt(internal_pointer); }

  NO_MOVE_COPY(XMLValidationContext)
};
