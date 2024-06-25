#pragma once

#include <nlohmann/json-schema.hpp>  // for basic_error_handler
#include <nlohmann/json.hpp>         // for basic_json
#include <nlohmann/json_fwd.hpp>     // for json
#include <string>                    // for string

struct JsonErrorHandler : nlohmann::json_schema::basic_error_handler {
  void error(const nlohmann::json::json_pointer &pointer_to_json,
             const nlohmann::json &json_instance,
             const std::string &message) override;
};

void show_parse_error(const std::string &message);
