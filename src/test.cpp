#include "Tester.h"     // for Tester
#include <QtCore/qglobal.h> // for qCritical
#include <qapplication.h>   // for QApplication
#include <qtestcase.h>      // for qExec

// first argument is the executable
// second argument is the examples folder
auto main(int number_of_arguments, char *arguments[]) -> int {
  if (number_of_arguments == 2) {
    QApplication const app(number_of_arguments, arguments);
    Tester tester(arguments[1]);
    // don't pass the test folder to the qExec
    auto just_one = 1;
    return QTest::qExec(&tester, just_one, arguments);
  }
  qCritical("Wrong number of arguments %i!", number_of_arguments);
  return -1;
}