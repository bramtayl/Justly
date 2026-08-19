#include "xml/XMLValidationContext.hpp"

#include <libxml/xmlschemas.h>

XMLValidationContext::~XMLValidationContext() {
  xmlSchemaFreeValidCtxt(internal_pointer);
}
