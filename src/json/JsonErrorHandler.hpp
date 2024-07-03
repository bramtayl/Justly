#pragma once

#include <qwidget.h>

#include <nlohmann/json-schema.hpp>  // for basic_error_handler
#include <nlohmann/json.hpp>         // for basic_json
#include <nlohmann/json_fwd.hpp>     // for json
#include <string>                    // for string

struct JsonErrorHandler : nlohmann::json_schema::basic_error_handler {
  QWidget* const parent_pointer;
  explicit JsonErrorHandler(QWidget* parent_pointer_input = nullptr);
  void error(const nlohmann::json::json_pointer &pointer_to_json,
             const nlohmann::json &json_instance,
             const std::string &message) override;
};

void show_parse_error(QWidget *parent_pointer, const std::string &message);
