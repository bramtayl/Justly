#include "Tester.h"     // for Tester
#include <QtCore/qglobal.h> // for qCritical
#include <qapplication.h>   // for QApplication
#include <qtestcase.h>      // for qExec

// first argument is the executable
// second argument is the examples folder
auto main(int number_of_arguments, char *arguments[]) -> int {
  if (number_of_arguments == 2) {
    // consume the examples folder, and only let qt use the first argument
    auto empty_number = 1;
    QApplication const app(empty_number, arguments);
    Tester tester("/home/brandon/Justly/src/orchestra.orc", "Plucked", arguments[1]);
    return QTest::qExec(&tester, empty_number, arguments);
  }
  qCritical("Wrong number of arguments %i!", number_of_arguments);
  return -1;
}