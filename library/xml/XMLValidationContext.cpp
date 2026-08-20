#include "xml/XMLValidationContext.hpp"

XMLValidationContext::~XMLValidationContext() {
  xmlSchemaFreeValidCtxt(internal_pointer);
}
