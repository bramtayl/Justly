#include "xml/XMLSchema.hpp"

XMLSchema::~XMLSchema() { xmlSchemaFree(internal_pointer); }
