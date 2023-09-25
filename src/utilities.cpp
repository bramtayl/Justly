#include "utilities.h"

#include <qmessagebox.h>  // for QMessageBox
#include <qobject.h>
#include <qstring.h>  // for QString, operator==, operator+

#include <nlohmann/json.hpp>

void show_open_error(const QString& filename) {
  QMessageBox::warning(nullptr, QObject::tr("File error"),
                       QObject::tr("Cannot open file \"%1\"!").arg(filename));
}

void show_parse_error(const nlohmann::json::parse_error& parse_error) {
  QMessageBox::warning(nullptr, QObject::tr("Parsing error"),
                       parse_error.what());
}
