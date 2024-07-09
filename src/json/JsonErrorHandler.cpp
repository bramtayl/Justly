#include "json/JsonErrorHandler.hpp"

#include <qassert.h>      // for Q_ASSERT
#include <qmessagebox.h>  // for QMessageBox
#include <qstring.h>      // for QString

#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for operator<<
#include <nlohmann/json-schema.hpp>          // for basic_error_handler
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <sstream>                           // for operator<<, basic_ostream

JsonErrorHandler::JsonErrorHandler(QWidget *parent_pointer_input)
    : parent_pointer(parent_pointer_input) {}

void JsonErrorHandler::error(
    const nlohmann::json::json_pointer &pointer_to_json,
    const nlohmann::json &json_instance, const std::string &message) {
  nlohmann::json_schema::basic_error_handler::error(pointer_to_json,
                                                    json_instance, message);
  std::stringstream error_message;
  Q_ASSERT(parent_pointer != nullptr);
  error_message << parent_pointer->tr("Context").toStdString() << ": \""
                << pointer_to_json << "\"" << std::endl
                << "JSON: \"" << json_instance << "\"" << std::endl
                << parent_pointer->tr("Message").toStdString() << ": "
                << message;
  QMessageBox::warning(parent_pointer, parent_pointer->tr("Schema error"),
                       QString::fromStdString(message));
}

