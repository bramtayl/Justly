#include <qtest.h>  // for QTEST_MAIN

#include "Tester.h"  // for Tester

#include "utilities.h"

int main(int number_of_arguments, char *arguments[]) {
    QApplication const app(number_of_arguments, arguments);
    auto soundfont_file = find_soundfont_file();
    if (soundfont_file.isNull()) {
        return 0;
    }
    Tester tester(soundfont_file);
    return QTest::qExec(&tester, number_of_arguments, arguments);
}
