#include "PyWindow.h"

#include <QApplication>
#include <QDir>



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    PyWindow mainWindow;
    mainWindow.setWindowTitle("Python Code Editor & Runner");
    mainWindow.show();

    return app.exec();
}
