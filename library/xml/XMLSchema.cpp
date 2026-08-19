#include "xml/XMLSchema.hpp"

#include <libxml/xmlschemas.h>

XMLSchema::~XMLSchema() { xmlSchemaFree(internal_pointer); }
